#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define POPULATION_SIZE 10
#define ITERATION_NUM 100000
//t:table   s:solution
int num_color=100;
int num_node=0;
int num_edge=0;
int **t_adjacent_half=NULL;//记录节点的比自身序号小的邻边
int **t_adjacent_all=NULL;//记录节点的所有邻边
int **t_population[POPULATION_SIZE];
int *l_s_best=NULL;
int *l_conflict_num=NULL;//长度为population_size+1，最后一个位置存子代冲突数
int **t_distance=NULL;//个体间距离表
int *l_min_distance=NULL;//个体与其它个体最小距离列表

int *l_s_curr=NULL;
int **t_conflict_table=NULL;
int **t_tabu_tenure=NULL;
/***********************************************/
//读取图信息，得到邻结点表
void read_file(void)
{
    int i;
    int j;
    int d_line_longth=120;
    int *is_list_adjacents;//记录相邻的边及个数，从而申请合适的空间
    int i_node_one;
    int i_node_two;
    char *cs_temp=NULL;
    char *cs_line=(char *)malloc(sizeof(char)*d_line_longth);
    FILE *f_instance=NULL;
    if(NULL=(f_instance=fopen(filename,"r"))){
        printf("read file error!");
        exit(-1);
    }
    while(NULL!=fgets(cs_line,d_line_longth,f_instance)){
        if('p'==cs_line[0]){
            cs_temp=strtok(cs_line," ");
            cs_temp=strtok(NULL," ");
            cs_temp=strtok(NULL," ");
            num_node=atoi(cs_temp);
            cs_temp=strtok(NULL," ");
            num_edge=atoi(cs_temp);
            break;
        }
    }
    is_list_adjacents=(int *)malloc(sizeof(int)*num_node);
    is_list_adjacents[0]=0;
    t_adjacent_half=(int **)malloc(sizeof(int*)*num_node);
    t_adjacent_all=(int **)malloc(sizeof(int*)*num_node);
    j=0;//记录进行到的结点序号
    //根据算例数据特点，可以记录每个结点对应的比自己序号小的邻结点的个数，然后申请个数+1的空间来记录邻结点信息
    while(NULL!=fgets(cs_line,d_line_longth,f_instance)){
        cs_temp=strtok(cs_line," ");
        cs_temp=strtok(NULL," ");
        i_node_one=atoi(cs_temp)-1;
        cs_temp=strtok(NULL," ");
        i_node_two=atoi(cs_temp)-1;
        if(j==i_node_one){
            is_list_adjacents[0]++;
            is_list_adjacents[is_list_adjacents[0]]=i_node_two;
        }
        else{
            t_adjacent_half[j]=(int *)malloc(sizeof(int)*(is_list_adjacents[0]+1));
            t_adjacent_half[j][0]=is_list_adjacents[0];
            for(i=0;i<is_list_adjacents[0];i++){
                t_adjacent_half[j][i+1]=is_list_adjacents[i+1];
            }
            j++;
            while(j<i_node_one){
                t_adjacent_half[j]=(int *)malloc(sizeof(int)*1);
                t_adjacent_half[j][0]=0;
                j++;
            }
            is_list_adjacents[0]=1;
            is_list_adjacents[1]=i_node_two;
        }
    }
    t_adjacent_half[j]=(int *)malloc(sizeof(int)*(is_list_adjacents[0]+1));
    t_adjacent_half[j][0]=is_list_adjacents[0];
    for(i=0;i<is_list_adjacents[0];i++){
        t_adjacent_half[j][i+1]=is_list_adjacents[i+1];
    }
    fclose(f_instance);
    if(num_node!=j){
        printf("j<num_node,the graph may be wrong!");
        exit(-1);
    }
    //利用得到的半邻边信息表，先算出每个结点的邻边数，构建空间，再记录具体邻边
    for(i=0;i<num_node;i++){
        is_list_adjacents[i]=0;
    }
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            is_list_adjacents[i]++;
            is_list_adjacents[t_adjacent_half[i][j+1]]++;
        }
    }
    for(i=0;i<num_node;i++){
        t_adjacent_all[i]=(int *)malloc(sizeof(int)*(is_list_adjacents[i]+1));
        t_adjacent_all[i][0]=0;
    }
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            t_adjacent_all[i][0]++;
            t_adjacent_all[i][t_adjacent_all[i][0]]=t_adjacent_half[i][j+1];
            t_adjacent_all[t_adjacent_half[i][j+1]][0]++;
            t_adjacent_all[t_adjacent_half[i][j+1]][t_adjacent_all[t_adjacent_half[i][j+1]][0]]=i;
        }
    }
}
//初始化种群，记录最优个体
void initial_populaion(void)
{
    int i;
    int j;
    int record[2];//最优解对应的个体序号，最优冲突数
    record[1]=num_node;
    srand((unsigned int)time(NULL));
    if(1==num_color){
        printf("the space of the t_population may be too small!");
        exit(-1);
    }
    //全局变量的初始化
    for(i=0;i<POPULATION_SIZE;i++){
        t_population[i]=(int **)malloc(sizeof(int *)*num_color);
        for(j=0;j<num_color;j++){
            t_population[i][j]=(int *)malloc(sizeof(int)*num_node);
            t_population[i][j][0]=0;
        }
    }
    l_conflict_num=(int *)malloc(sizeof(int)*(POPULATION_SIZE+1));
    l_s_curr=(int *)malloc(sizeof(int)*num_node);
    l_s_best=(int *)malloc(sizeof(int)*num_node);
    t_conflict_table=(int **)malloc(sizeof(int *)*num_node);
    for(i=0;i<num_node;i++){
        t_conflict_table[i]=(int *)malloc(sizeof(int)*num_color);
    }
    t_tabu_tenure=(int **)malloc(sizeof(int *)*num_node);
    for(i=0;i<num_node;i++){
        t_tabu_tenure[i]=(int *)malloc(sizeof(int)*num_color);
    }
    for(i=0;i<POPULATION_SIZE;i++){
        for(j=0;j<num_node;j++){
            l_s_curr[j]=rand()%num_color;
        }
        tabu_search(l_s_curr,&l_conflict_num[i]);
        for(j=0;j<num_node;j++){
            t_population[i][l_s_curr[j]][0]++;
            t_population[i][l_s_curr[j]][t_population[i][l_s_curr[j]][0]]=j;
        }
        if(record>l_conflict_num[i]){
            record[0]=i;
            record[1]=l_conflict_num[i];
        }
    }
    //记录最优解
    for(i=0;i<num_color;i++){
        for(j=0;j<t_population[record[0]][i][0];j++){
            l_s_best[t_population[record[0]][i][j+1]]=i;
        }
    }
    //初始化距离表
    int *classes_one=(int *)malloc(sizeof(int)*num_color);
    int *classes_two=(int *)malloc(sizeof(int)*num_color);
    int **match_table=
    for(i=0;i<POPULATION_SIZE-1;i++){
        for(j=i;j<POPULATION_SIZE;j++){

        }
    }
}
//禁忌搜索，传入解及冲突数返回优化后的解及冲突数
void tabu_search(int *solution,int &conflice_num)
{
    int i;
    int j;
    int m;
    int color_before;
    int color_after;
    int record_nontabu[2];//结点，变化后的颜色
    int record_tabu[2];
    int choice_num_tabu;
    int choice_num_nontabu;
    int curr_conflict_num=0;
    int delta_nontabu=0;
    int delta_tabu=0;
    int temp_delta;
    int best_conflict=num_node;
    int *best_solution=(int *)malloc(sizeof(int)*num_node);
    //初始化变量
    for(i=0;i<num_node;i++){
        best_solution[i]=solution[i];
        for(j=0;j<num_color;j++){
            t_conflict_table[i][j]=0;
            t_tabu_tenure[i][j]=0;
        }
    }
    //构建冲突表
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            if(solution[i]==solution[t_adjacent_half[i][j+1]]){
                t_conflict_table[i][solution[i]]++;
                t_conflict_table[t_adjacent_half[i][j+1]][solution[t_adjacent_half[i][j+1]]]++;
                curr_conflict_num++;
            }
        }
    }
    //TS algorithm  i迭代次数,j结点,m颜色
    for(i=0;i<ITERATION_NUM;i++){
        delta_nontabu=num_node;
        delta_tabu=num_node;
        choice_num_tabu=0;
        choice_num_nontabu=0;
        //探索领域解
        for(j=0;j<num_node;j++){
            //只考虑有冲突的结点
            if(0<t_conflict_table[j][solution[j]){
                for(m=1;m<num_color;m++){
                    color_before=solution[j];
                    color_after=(solution[j]+m)%num_color;
                    temp_delta=t_conflict_table[j][color_before]-t_conflict_table[j][color_after];
                    //对禁忌与非禁忌的情况分别处理并记录
                    if(t_tabu_tenure[j][color_after]<i){
                        if(delta_nontabu<temp_delta){
                            continue;
                        }
                        else{
                            if(delta_nontabu==temp_delta){
                                choice_num_nontabu++;
                                if(0==rand()%choice_num_nontabu){
                                    record_nontabu[0]=j;
                                    record_nontabu[1]=color_after;
                                }
                            }
                            else{
                                record_nontabu[0]=j;
                                record_nontabu[1]=color_after;
                                choice_num_nontabu=1;
                            }
                        }
                    }
                    else{
                        if(delta_nontabu>temp_delta&&((best_conflict>=curr_conflict_num+temp_delta)||0==choice_num_nontabu)){
                            if(delta_tabu<temp_delta){
                                continue;
                            }
                            else{
                                if(delta_tabu==temp_delta){
                                    choice_num_tabu++;
                                    if(0==rand()%choice_num_tabu){
                                        record_tabu[0]=j;
                                        record_tabu[1]=color_after;
                                    }
                                }
                                else{
                                    record_tabu[0]=j;
                                    record_tabu[1]=color_after;
                                    choice_num_tabu=1;
                                }
                            }
                        }
                    }
                }
            }
        }
        //选择好的移动方法，进行移动
        if(0==choice_num_nontabu){
            j=record_tabu[0];
            color_before=solution[j];
            color_after=record_tabu[1];
        }
        else{
            if(delta_tabu<delta_nontabu&&(delta_tabu+curr_conflict_num)<=best_conflict){
                j=record_tabu[0];
                color_before=solution[j];
                color_after=record_tabu[1];
            }
            else{
                j=record_nontabu[0];
                color_before=solution[j];
                color_after=record_nontabu[1];
            }
        }
        solution[j]=color_after;
        curr_conflict_num-=t_conflict_table[j][color_before];
        curr_conflict_num+=t_conflict_table[j][color_after];
        for(m=0;m<t_adjacent_all[j][0];m++){
            curr_conflict_num-=t_conflict_table[t_adjacent_all[j][m+1]][solution[t_adjacent_all[j][m+1]]];
            t_conflict_table[t_adjacent_all[j][m+1]][color_before]--;
            t_conflict_table[t_adjacent_all[j][m+1]][color_after]++;
            curr_conflict_num+=t_conflict_table[t_adjacent_all[j][m+1]][solution[t_adjacent_all[j][m+1]]];
        }
        t_tabu_tenure[j][color_before]=i+curr_conflict_num+rand()%10;
        if(curr_conflict_num<=best_conflict){
            for(m=0;m<num_node;m++){
                best_solution[i]=solution[i];
            }
            best_conflict=curr_conflict_num;
        }
    }
    //返回优化的解
    *conflice_num=best_conflict;
    for(i=0;i<num_node;i++){
        solution[i]=best_solution[i];
    }
}
//动态多父本交叉算子。传入种群、参与交叉的父本数、对应有父本序号、返回的子代的解。
void adaptive_multi_parent_crossover(int ***population,int parent_num,int *parents,int *child)
{
    int i;
    int j;
    int m;
    int n;
    int temp_parent_num;
    int temp_parent;
    int temp_node;
    int temp_color;
    int **population_parents[parent_num];//新建空间存储父本种群
    int *solution_child=(int *)malloc(sizeof(int)*num_node);//产生的子代
    int *solution_parents[parent_num];//父本对应的一维解集
    int *solution_parents_node_position[parent_num];//父本对应的一维解集的结点在二维中的位置
    int max_classes[parent_num][2];//结点数，对应class
    for(i=0;i<parent_num;i++){
        max_classes[i][0]=0;
    }
    for(i=0;i<parent_num;i++){
        solution_parents[i]=(int *)malloc(sizeof(int)*num_node);
        solution_parents_node_position[i]=(int *)malloc(sizeof(int)*num_node);
    }
    for(i=0;i<parent_num;i++){
        population_parents[i]=(int **)malloc(sizeof(int *)*num_color);
        for(j=0;j<num_color;j++){
            population_parents[i][j]=(int *)malloc(sizeof(int)*num_node);
            population_parents[i][j][0]=population[parents[i]][j][0];
            for(m=0;m<population[parents[i]][j][0];m++){
                population_parents[i][j][m+1]=population[parents[i]][j][m+1];
                solution_parents[i][population[parents[i]][j][m+1]]=j;
                solution_parents_node_position[i][population[parents[i]][j][m+1]]=m+1;
            }
            if(max_classes[i][0]<population[parents[i]][j][0]){
                max_classes[i][0]=population[parents[i]][j][0];
                max_classes[i][1]=j;
            }
        }
    }
    //交叉算子
    for(i=0;i<num_color;i++){
        //找到最大子集
        m=max_classes[0][0];
        temp_parent_num=0;
        for(j=1;j<parent_num;j++){
            if(m<max_classes[j][0]){
                m=max_classes[j][0];
                temp_parent_num=j;
            }
        }
        //将子集复制到子代中,去除父本中用过的结点.
        for(j=0;j<population_parents[parents[temp_parent_num]][max_classes[temp_parent_num][1]][0];j++){
            temp_node=population_parents[parents[temp_parent_num]][max_classes[temp_parent_num][1]][j+1];
            solution_child[temp_node]=i;
            for(m=1;m<parent_num;m++){
                temp_parent=(temp_parent_num+m)%parent_num;
                temp_color=solution_parents[temp_parent][temp_node];
                for(n=solution_parents_node_position[temp_parent][temp_node];n<population_parents[temp_parent][temp_color][0];n++){
                    population_parents[temp_parent][temp_color][n]=population_parents[temp_parent][temp_color][n+1];
                }
                population_parents[temp_parent][temp_color][0]--;
            }
        }
        population_parents[parents[temp_parent_num]][max_classes[temp_parent_num][1]][0]=0;
        //更新辅助记录信息
        for(j=0;j<parent_num;j++){
            max_classes[j][0]=population_parents[j][0][0];
            max_classes[j][1]=0;
            for(m=1;m<num_color;m++){
                if(max_classes[j][0]<population_parents[j][m][0]){
                    max_classes[j][0]=population_parents[j][m][0];
                    max_classes[j][1]=m;
                }
            }
        }
    }
    //剩余结点随机分配
    if(max_classes[0][0]>0){
        for(i=0;i<num_color;i++){
            for(j=0;j<population_parents[0][i][0];j++){
                solution_child[population_parents[0][i][j+1]]=rand()%num_color;
            }
        }
    }
    //返回结果
    for(i=0;i<num_node;i++){
        child[i]=solution_child[i];
    }
}
//更新种群。先用最简单的方法，根据目标函数大小，用子代替换较差的父本
void pool_updating()
{
}
//记录结果
void write_file(int *solution)
{
    FILE *f_solution=NULL;
    int i;
    if((f_solution=fopen("out.txt","w"))==NULL){
        printf("fopen out error!");
        exit(-1);
    }
    for(i=0;i<num_node;i++){
        fprintf(f_solution,"%d ",l_s_best[i]);
    }
    fclose(out);
    printf("write answer\n");
}
//对解进行检查，输入解，输出结果
void check_answer(int *solution)
{
    int i;
    int j;
    int flag=1;
    printf("check:\n");
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            if(solution[i]==solution[t_adjacent_half[i][j+1]]){
                printf("%d,%d should not have the same color!\n");
                flag=0;
            }
        }
    }
    if(1==flag){
        printf("the answer is right!\n");
    }
}
/***********************************************/
int main()
{
    int i;

    read_file();
    initial_populaion();
    while(1){
        if()
    }

    return 0;
}
