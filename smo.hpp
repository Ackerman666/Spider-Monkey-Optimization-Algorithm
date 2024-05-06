#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <string>
#include <chrono>
#include "config.hpp"
#include "test_function.hpp"

using namespace std;

typedef vector<double> solution;
random_device rd;
mt19937 gen(rd());

// random floating number in [min, max]
double uniform_rand(int begin, int end){

    uniform_real_distribution<> dis(begin, end);
    return dis(gen);

}

// random integer in [min, max]
int rand_int(int min, int max) {

    std::uniform_int_distribution<> dis(min, max);
    return dis(gen); 
}



double inline rouletteProb(double &f, double &max_f){
    //cout<< 0.9 * (f / max_f) + 0.3 <<endl;
    return 0.9 * (f / max_f) + 0.1;
}


class LocalGroup{
    public :
        // ll_count :　local limit count
        int from, end, leader_idx, ll_count, size; 
        double leader_fitness;

        LocalGroup(int f, int ed){ init(f, ed); }
    private :
        void init(int f, int ed);
};

inline void LocalGroup::init(int f, int ed){
    from = f;
    end = ed;
    leader_idx = -1;
    ll_count = 0;
    leader_fitness = -1;
    size = end - from + 1;
}





// spider monkey optimization
class SMO
{
public:
    SMO(int ll, int gl, int ps, double pr);

    double run(string func, int d, int r);

private:

    /*
     * local leader limit
     * global leader limit
     * global limit count
     * population_size
     * current local group count
    */ 
    int local_limit, global_limit, gl_count, population_size, lg_size;
    // perturbation rate
    double pr;
    // population
    vector<solution> spider_monkeys;

    // global leader
    double global_leader_fitness;
    solution global_leader;

    // local leader
    vector<LocalGroup> local_group;

     //fitness
    vector<double> fitness;

    
    void init(double ub, double lb, int d, string objf);


    void localPhase(int d, string objf);
    void globalPhase(int d, string objf);
    void globalLearning();
    void localLearning();
    void localDecision(int d, double ub, double lb);
    void globalDecision();

    // update fitness value for each spider monkey
    void   updateFitness(int from, int end, int d, string objf);
    double calculateFitness(double funv);

    // select group partner excluded "idx"
    int partnerSelect(LocalGroup &lg, int idx);

};


inline SMO::SMO(int ll, int gl, int ps, double pr){
    local_limit = ll;
    global_limit = gl;
    population_size = ps;
    pr = pr;
}


int inline SMO::partnerSelect(LocalGroup &lg, int idx){
    int select = idx;
    while(select == idx)
        select = rand_int(lg.from, lg.end);

    return select;
}

/*
void inline SMO::updateFitness(int from, int end, int d, string objf){

    for(int i = from ; i <= end ; i++){
        double funv = TestFunctions::calculate_test_function(spider_monkeys[i], d, TEST_FUNCTION[objf]);
        double fit = calculateFitness(funv);
        if(fit > fitness[i])
            fitness[i] = fit;
    }

    return;
}
*/

double inline SMO::calculateFitness(double funv){
    if (funv >= 0)
        return 1 / (funv + 1);
    else
        return 1 + fabs(funv);

}

void inline SMO::init(double ub, double lb, int d, string objf){

    gl_count = 0;
    global_leader_fitness = -1;
    
    // initialize MAX_GROUP_SIZE empty local group
    for(int i=0 ; i<MAX_GROUP_SIZE ; i++)
        local_group.push_back(LocalGroup(0,0));

   
    spider_monkeys.resize(population_size);

    for(int i=0 ; i<population_size ; i++){
        for(int j=0 ; j<d ; j++){
            spider_monkeys[i].push_back(lb + uniform_rand(0,1) * (ub - lb));
        }
    }

    for(int i=0 ; i<population_size ; i++){
        double funv = TestFunctions::calculate_test_function(spider_monkeys[i], d, TEST_FUNCTION[objf]);
        double fit = calculateFitness(funv);
        fitness.push_back(fit);
    }

    LocalGroup local(0, population_size-1);
    local_group[0] = local;
    lg_size = 1;




    /*
    //todo : look output
    for(int i=0 ; i<population_size ; i++){

        cout<<i<<" : [ ";

        for(int j=0 ; j<d ; j++){

            cout<<spider_monkeys[i][j]<<", ";

        }
        cout<<" ] "<<endl;
    
    }
    */
    
    //

    /*
    //todo : look fitness
    cout<<"fitness : \n";
    for(int i=0 ; i<population_size ; i++){

        cout<<i<<" : [ ";
        cout<<fitness[i];
        cout<<" ] "<<endl;
    }
    */
    


}


void inline SMO::globalLearning(){

    solution tmp = global_leader;
    for(int i=0 ; i<population_size ; i++){
        if(fitness[i] > global_leader_fitness){
            tmp = spider_monkeys[i];
            global_leader_fitness = fitness[i];
        }
    }

    if(tmp == global_leader)
        gl_count += 1;
    else{
        gl_count = 0;
        global_leader = tmp;
    }

    /*
    // for test
    int len = global_leader.size();
    cout<<"Global leader : [ ";
    for(auto i=0 ; i<len ; i++){
        cout<<global_leader[i]<<", ";
    }
    cout<<" ] fitness : "<<global_leader_fitness<<endl;
    */
    
}

void inline SMO::localLearning(){

    for(int i=0 ; i<lg_size ; i++){
        LocalGroup *local = &local_group[i];
        int tmp = local->leader_idx;

        for(int j=local->from ; j<=local->end ; j++){
            if(fitness[j] > local->leader_fitness){
                local->leader_fitness = fitness[j];
                tmp = j;
            }
        }

        if(tmp == local->leader_idx)
            local_group[i].ll_count += 1;

        else{
            local->ll_count = 0;
            local->leader_idx = tmp;
        }
    }
}

void inline SMO::globalPhase(int d, string objf){


    //each group do globalPhase
    for(int i=0 ; i<lg_size ; i++){
        
        LocalGroup *local = &local_group[i];
        int count = 0;

        vector<solution> tmp_group;
        tmp_group.reserve(local->size);
        while(count < local->size){
            
            for(int j=local->from ; j<=local->end ; j++){     
                double ur1 = uniform_rand(0,1), ur2 = uniform_rand(-1,1);
                solution tmp = spider_monkeys[j];
                //ur1 < (rouletteProb(fitness[j], global_leader_fitness))
                if(ur1 < rouletteProb(fitness[j], global_leader_fitness) + 0.3){
                    
                    // selected dimension
                    int sd = rand_int(0, d-1);
                    int partner_idx = partnerSelect(local_group[i], j);
                    tmp[sd] = tmp[sd] + ur1 * (global_leader[sd] - tmp[sd])  + ur2 * (spider_monkeys[partner_idx][sd] - tmp[sd]);
                    count += 1;

                    tmp_group.push_back(tmp);
                    
                }

            }
        }

        for(int i=0, j=local->from ; j<=local->end ; j++, i++){
            double funv = TestFunctions::calculate_test_function(tmp_group[i], d, TEST_FUNCTION[objf]);
            double fit = calculateFitness(funv);

            //et higher fitness, update position
            if(fitness[j] < fit){
                fitness[j] = fit;
                spider_monkeys[j] = tmp_group[i];
            }
        }
       
    }
}

void inline SMO::localPhase(int d, string objf){

    for(int i=0 ; i<lg_size ; i++){

        LocalGroup *local = &local_group[i];
        
        for(int j=local->from ; j<=local->end ; j++){
            
            solution tmp = spider_monkeys[j];

            // 抓最新的位置可能為何
            for(int k=0 ; k<d ; k++){
                double ur1 = uniform_rand(0,1), ur2 = uniform_rand(-1,1);

                if(ur1 >= pr){
                    int partner_idx = partnerSelect(local_group[i], j);
                    tmp[k] = tmp[k] + ur1 * (spider_monkeys[local->leader_idx][k] - tmp[k]) + ur2 * (spider_monkeys[partner_idx][k] - tmp[k]);
                    //spider_monkeys[j] = tmp;
                }
            }

            double funv = TestFunctions::calculate_test_function(tmp, d, TEST_FUNCTION[objf]);
            double fit = calculateFitness(funv);
            
            // get higher fitness, update position
            if(fitness[j] < fit){
                fitness[j] = fit;
                spider_monkeys[j] = tmp;
            }
            

        }       
    }

}

void inline SMO::localDecision(int d, double ub, double lb){


    for(int i=0 ; i<lg_size ; i++){

        LocalGroup *local = &local_group[i];

        // fall into local minimun
        if(local->ll_count > local_limit){

            local->ll_count = 0;

            for(int j=local->from ; j<=local->end ; j++){

                for(int k=0 ; k<d ; k++){

                    double ur1 = uniform_rand(0,1);
                    
                    //todo 亂數部份論文是重新生成一個
                    if(ur1 >= pr)
                        spider_monkeys[j][k] = lb + ur1  * (ub - lb);
                    else
                        spider_monkeys[j][k] = spider_monkeys[j][k] + ur1 * (global_leader[k] - spider_monkeys[j][k]) + ur1 * (spider_monkeys[j][k] - spider_monkeys[local->leader_idx][k]);
                }
            
            }
        }
    }
}

void inline SMO::globalDecision(){

    if(gl_count > global_limit){

        gl_count = 0;

        if(lg_size < MAX_GROUP_SIZE){

            lg_size = lg_size << 1;
            int interval = population_size / lg_size;

            for(int i=0, from = 0 ; i<lg_size ; from+=interval, i++){
                LocalGroup local(from, from+interval-1);
                local_group[i] = local;
            }

        }
        else{
            lg_size = 1;
            LocalGroup local(0, population_size-1);
            local_group[0] = local;
        }
        localLearning();

    }
}





/* run SMO by "func" as optimization object
 *
 * @ epoch : run times
 * @ evalutions : evalutions times per run
 * @ func : object function
 * @ d : dimension of object funciton
 *  
 */
double inline SMO::run(string objf, int d, int r){

    vector<double> bound = TestFunctions::get_search_bound(TEST_FUNCTION[objf]);
    init(bound[0], bound[1], d, objf);
    globalLearning();
    localLearning();

    int evalutions = BASE_EVALUATION * d;
    //int evalutions = 20;


    for(int i=1 ; i<=evalutions ; i++){
        localPhase(d, objf);
        globalPhase(d, objf);
        globalLearning();
        localLearning();
        localDecision(d, bound[0], bound[1]);
        globalDecision();

        //cout<<"[ "<<i<<" ] : "<<TestFunctions::calculate_test_function(global_leader, d, TEST_FUNCTION[objf])<<endl;

    }

    double optimize_val = TestFunctions::calculate_test_function(global_leader, d, TEST_FUNCTION[objf]);
    cout<<"Run "<<r<<", Get optimize value : "<<optimize_val<<endl;
    /*
    int len = global_leader.size();
    cout<<"Global leader : [ ";
    for(auto i=0 ; i<len ; i++){
        cout<<global_leader[i]<<", ";
    }
    cout<<" ] fitness : "<<global_leader_fitness<<endl;
    */

    return optimize_val;
}