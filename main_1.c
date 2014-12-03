#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITERATION_NUM 8000
#define TEST_NUM 30
#define BIG_CICLE_NUM 10 //大的周期数
#define PARENTS_NUM 2
/***********************************************/
#define THE_CHOICE 5
int colors[]={5,17,44,8,28,72,12,50,49,48};
char *instances[]={"dsjc125.1.col",//0_5
                   "dsjc125.5.col",//1_17
                   "dsjc125.9.col",//2_44
                   "dsjc250.1.col",//3_8
                   "dsjc250.5.col",//4_28
                   "dsjc250.9.col",//5_72
                   "dsjc500.1.col",//6_12
                   "dsjc500.5.col",//7_50
                   "dsjc500.5.col",//8_49
                   "dsjc500.5.col",//9_48
                    };
char *filename_in;
char *filename_out;
char *filename_run_data;
//t:table   s:solution
int num_color=0;
int num_node=0;
int num_edge=0;
int **t_adjacent_half=NULL;//记录节点的比自身序号小的邻边
int **t_adjacent_all=NULL;//记录节点的所有邻边
int **t_conflict_table=NULL;
int **t_tabu_tenure=NULL;
/*************************/
int **parents[PARENTS_NUM];
int *children[PARENTS_NUM];
int *best_solution;
int *history_solution;
int parents_conflict[PARENTS_NUM];
int children_conflict[PARENTS_NUM];
int best_conflict;
int history_conflict;
//统计数据
clock_t run_begin;
clock_t run_end;
double run_time;
int generation;
time_t m_time;
struct tm *p_time_struct=NULL;
/***********************************************/
//对输入输出的文件名初始化，方便算例的选择，方便操作
void initial_program(void)
{
    //filename
    filename_in=(char *)malloc(sizeof(char)*50);
    filename_out=(char *)malloc(sizeof(char)*50);
    filename_run_data=(char *)malloc(sizeof(char)*50);

    int choice=THE_CHOICE;
    char *path_in="../../Instances/";
    char *path_out="output/";
    char *temp=(char *)malloc(sizeof(char)*10);
    //color num
    num_color=colors[choice];
    //input
    strcpy(filename_in,path_in);
    strcat(filename_in,instances[choice]);
    //output
    itoa(num_color,temp,10);
    strcpy(filename_out,path_out);
    strncat(filename_out,instances[choice],9);
    strcat(filename_out,"_");
    strcat(filename_out,temp);
    strcat(filename_out,".txt");
    //run data
    strcpy(filename_run_data,path_out);
    strcat(filename_run_data,"run_data.txt");

    //other
    generation=0;
    time(&m_time);
    p_time_struct=localtime(&m_time);
    run_begin=clock();
    srand((unsigned int)time(NULL));
}
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
    if(NULL==(f_instance=fopen(filename_in,"r"))){
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
    if(num_node!=j+1){
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
//禁忌搜索，传入解及冲突数返回优化后的解及冲突数
void tabu_search(int *solution,int *conflict_num)
{
    int i;
    int j;
    int m;
    int temp_node;
    int temp_color;
    int color_before;
    int color_after;
    int record_nontabu[2]={-1,-1};//结点，变化后的颜色
    int record_tabu[2]={-1,-1};
    int choice_num_tabu;
    int choice_num_nontabu;
    int curr_conflict_num=0;
    int delta_nontabu=0;
    int delta_tabu=0;
    int temp_delta;
    int best_conflict=num_node*num_node;
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
        for(j=0;j<t_adjacent_all[i][0];j++){
            temp_node=t_adjacent_all[i][j+1];
            temp_color=solution[i];
            t_conflict_table[temp_node][temp_color]++;
        }
    }
    for(i=0;i<num_node;i++){
        temp_node=i;
        temp_color=solution[temp_node];
        curr_conflict_num+=t_conflict_table[temp_node][temp_color];//是实际冲突数的两倍
    }
    curr_conflict_num/=2;
    //TS algorithm  i迭代次数,j结点,m颜色
    //for(i=0;1;i++){
    for(i=0;i<ITERATION_NUM;i++){
        delta_nontabu=num_node;
        delta_tabu=num_node;
        choice_num_tabu=0;
        choice_num_nontabu=0;
        //探索领域解
        for(j=0;j<num_node;j++){
            //只考虑有冲突的结点
            temp_node=j;
            temp_color=solution[temp_node];
            if(0<t_conflict_table[temp_node][temp_color]){
                for(m=1;m<num_color;m++){
                    color_before=temp_color;
                    color_after=(temp_color+m)%num_color;
                    temp_delta=t_conflict_table[temp_node][color_after]-t_conflict_table[temp_node][color_before];
                    //对禁忌与非禁忌的情况分别处理并记录
                    if(t_tabu_tenure[temp_node][color_after]<i){
                        if(temp_delta<delta_nontabu){
                            record_nontabu[0]=temp_node;
                            record_nontabu[1]=color_after;
                            choice_num_nontabu=1;
                            delta_nontabu=temp_delta;
                        }
                        else{
                            if(delta_nontabu==temp_delta){
                                choice_num_nontabu++;
                                if(0==rand()%choice_num_nontabu){
                                    record_nontabu[0]=temp_node;
                                    record_nontabu[1]=color_after;
                                }
                            }
                        }
                    }
                    else{
                        if(temp_delta<delta_tabu){
                            record_tabu[0]=temp_node;
                            record_tabu[1]=color_after;
                            choice_num_tabu=1;
                            delta_tabu=temp_delta;
                        }
                        else{
                            if(delta_tabu==temp_delta){
                                choice_num_tabu++;
                                if(0==rand()%choice_num_tabu){
                                    record_tabu[0]=temp_node;
                                    record_tabu[1]=color_after;
                                }
                            }
                        }
                    }
                }
            }
        }
        //选择好的移动方法，进行移动
        if((curr_conflict_num+delta_tabu<best_conflict && delta_tabu<delta_nontabu)||(choice_num_nontabu==0)){
            temp_node=record_tabu[0];
            color_after=record_tabu[1];
            temp_delta=delta_tabu;
        }
        else{
            temp_node=record_nontabu[0];
            color_after=record_nontabu[1];
            temp_delta=delta_nontabu;
        }
        color_before=solution[temp_node];
        solution[temp_node]=color_after;
        curr_conflict_num+=temp_delta;
        for(m=0;m<t_adjacent_all[temp_node][0];m++){
            t_conflict_table[t_adjacent_all[temp_node][m+1]][color_before]--;
            t_conflict_table[t_adjacent_all[temp_node][m+1]][color_after]++;
        }
        t_tabu_tenure[temp_node][color_before]=i+curr_conflict_num+rand()%10;
        if(curr_conflict_num<best_conflict){
            for(m=0;m<num_node;m++){
                best_solution[m]=solution[m];
            }
            best_conflict=curr_conflict_num;
        }
        if(best_conflict==0){
            break;
        }
    }
    //返回优化的解
    *conflict_num=best_conflict;
    for(i=0;i<num_node;i++){
        solution[i]=best_solution[i];
    }
}
//不同类型解之间的复制
void copy_solution_oneTotwo(int **solution,int *new_solution){
    int i;
    int node;
    int color;
    int position;
    for(i=0;i<num_color;i++){
        solution[i][0]=0;
    }
    for(i=0;i<num_node;i++){
        node=i;
        color=new_solution[node];
        solution[color][0]++;
        position=solution[color][0];
        solution[color][position]=node;
    }
}
void copy_solution_oneToone(int *solution,int *new_solution){
    int i;
    for(i=0;i<num_node;i++){
        solution[i]=new_solution[i];
    }
}
void copy_solution_twoToone(int *solution,int **new_solution){
    int i;
    int j;
    int node;
    int color;
    int position;
    for(i=0;i<num_color;i++){
        color=i;
        for(j=0;j<new_solution[color][0];j++){
            position=j+1;
            node=new_solution[color][position];
            solution[node]=color;
        }
    }
}
//初始化种群，记录最优个体
void initial_populaion(void)
{
    int i;
    int j;
    int *solution=(int *)malloc(sizeof(int)*num_node);
    int record[2];
    record[0]=0;//the best parent
    record[1]=num_node*num_node;//the best conflict
    for(i=0;i<PARENTS_NUM;i++){
        parents[i]=(int **)malloc(sizeof(int *)*num_color);
        children[i]=(int *)malloc(sizeof(int)*num_node);
        for(j=0;j<num_color;j++){
            parents[i][j]=(int *)malloc(sizeof(int)*num_node);
            parents[i][j][0]=0;
        }
    }
    history_solution=(int *)malloc(sizeof(int)*num_node);
    best_solution=(int *)malloc(sizeof(int)*num_node);
    t_conflict_table=(int **)malloc(sizeof(int *)*num_node);
    t_tabu_tenure=(int **)malloc(sizeof(int *)*num_node);
    for(i=0;i<num_node;i++){
        t_conflict_table[i]=(int *)malloc(sizeof(int)*num_color);
        t_tabu_tenure[i]=(int *)malloc(sizeof(int)*num_color);
    }
    //初始化父母
    for(i=0;i<PARENTS_NUM;i++){
        for(j=0;j<num_node;j++){
            solution[j]=rand()%num_color;
        }
        tabu_search(solution,&parents_conflict[i]);
        copy_solution_oneTotwo(parents[i],solution);
        if(parents_conflict[i]<=0){
            copy_solution_twoToone(best_solution,parents[i]);
            best_conflict=parents_conflict[i];
            return;
        }
        if(parents_conflict[i]<record[1]){
            record[0]=i;
            record[1]=parents_conflict[i];
        }
    }
    //记录最优解
    copy_solution_twoToone(best_solution,parents[record[0]]);
    best_conflict=parents_conflict[record[0]];
    copy_solution_twoToone(history_solution,parents[record[0]]);
    history_conflict=parents_conflict[record[0]];
}
//返回包含结点数最多的颜色号
int max_class(int **solution)
{
    int i;
    int n=-1;//color
    int m=-1;//num of node
    for(i=0;i<num_color;i++){
        if(m<solution[i][0]){
            n=i;
            m=solution[i][0];
        }
    }
    return n;
}
//两个母本交叉产生子代，先从一特定母本中找最大子集进行遗传
void crossover(int ***population_parents,int main_parent,int *solution_child)
{
    int i;
    int j;
    int m;
    int n;
    int temp_parent;
    int temp_node;
    int temp_color;
    int temp_position;
    int choosed_parent;
    int **temp_population[PARENTS_NUM];
    int *solution_parents[PARENTS_NUM];//父本对应的一维解集
    int *node_positionInpopulation_parents[PARENTS_NUM];//父本对应的一维解集的结点在二维中的位置
    int bigest_class;//最多结点数对应class
    //初始化辅助信息变量
    for(i=0;i<PARENTS_NUM;i++){
        temp_population[i]=(int **)malloc(sizeof(int *)*num_color);
        for(j=0;j<num_color;j++){
            temp_population[i][j]=(int *)malloc(sizeof(int)*num_node);
        }
        solution_parents[i]=(int *)malloc(sizeof(int)*num_node);
        node_positionInpopulation_parents[i]=(int *)malloc(sizeof(int)*num_node);
    }
    for(i=0;i<PARENTS_NUM;i++){
        temp_parent=i;
        for(j=0;j<num_color;j++){
            temp_color=j;
            temp_population[temp_parent][temp_color][0]=population_parents[temp_parent][temp_color][0];
            for(m=0;m<population_parents[temp_parent][temp_color][0];m++){
                temp_position=m+1;
                temp_node=population_parents[temp_parent][temp_color][temp_position];
                temp_population[temp_parent][temp_color][temp_position]=population_parents[temp_parent][temp_color][temp_position];
                solution_parents[temp_parent][temp_node]=temp_color;
                node_positionInpopulation_parents[temp_parent][temp_node]=temp_position;
            }
        }
    }
    choosed_parent=main_parent;
    bigest_class=max_class(temp_population[choosed_parent]);//找到最大子集
    //交叉算子
    for(i=0;i<num_color;i++){
        //将子集复制到子代中,去除父本中用过的结点.
        for(j=0;j<temp_population[choosed_parent][bigest_class][0];j++){
            temp_position=j+1;
            temp_node=temp_population[choosed_parent][bigest_class][temp_position];
            solution_child[temp_node]=i;
            //对其它父本进行处理，移除该结点
            for(m=1;m<PARENTS_NUM;m++){
                temp_parent=(choosed_parent+m)%PARENTS_NUM;
                temp_color=solution_parents[temp_parent][temp_node];
                for(n=node_positionInpopulation_parents[temp_parent][temp_node];n<temp_population[temp_parent][temp_color][0];n++){
                    temp_population[temp_parent][temp_color][n]=temp_population[temp_parent][temp_color][n+1];
                    node_positionInpopulation_parents[temp_parent][temp_population[temp_parent][temp_color][n+1]]--;
                }
                temp_population[temp_parent][temp_color][0]--;
            }
        }
        temp_population[choosed_parent][bigest_class][0]=0;
        //更新辅助记录信息
        choosed_parent=(choosed_parent+1)%PARENTS_NUM;
        bigest_class=max_class(temp_population[choosed_parent]);
    }
    //剩余结点随机分配
    if(temp_population[choosed_parent][bigest_class][0]>0){
        for(i=0;i<num_color;i++){
            temp_color=i;
            for(j=0;j<temp_population[choosed_parent][temp_color][0];j++){
                temp_position=j+1;
                temp_node=temp_population[choosed_parent][temp_color][temp_position];
                solution_child[temp_node]=rand()%num_color;
            }
        }
    }
}
//save best and replace
void updating(int generation)
{
    int i;
    int m_bad=-1;//冲突
    int n_bad=-1;//对应的母本
    //优化子代并替换母本
    for(i=0;i<PARENTS_NUM;i++){
        tabu_search(children[i],&children_conflict[i]);
        if(best_conflict>children_conflict[i]){
            copy_solution_oneToone(best_solution,children[i]);
            best_conflict=children_conflict[i];
            if(best_conflict<=0){
                return;
            }
        }
        copy_solution_oneTotwo(parents[i],children[i]);
        parents_conflict[i]=children_conflict[i];
    }
    //每个大周期结束时用上个大周期的最优解替换现在的一个母本
    if(generation==BIG_CICLE_NUM){
        //找到当前母本里最差的个体
        for(i=0;i<PARENTS_NUM;i++){
            if(m_bad<children_conflict[i]){
                n_bad=i;
                m_bad=children_conflict[i];
            }
        }
        //替换
        copy_solution_oneTotwo(parents[n_bad],history_solution);
        parents_conflict[n_bad]=history_conflict;

        copy_solution_oneToone(history_solution,best_solution);
        history_conflict=best_conflict;
    }
}
//记录结果
void write_file(int *solution)
{
    run_end=clock();
    run_time=(double)(run_end-run_begin)/CLOCKS_PER_SEC;
    //记录解集
    FILE *f_solution=NULL;
    int i;
    if((f_solution=fopen(filename_out,"w"))==NULL){
        printf("fopen out error!");
        exit(-1);
    }
    for(i=0;i<num_node;i++){
        fprintf(f_solution,"%d %d\n",i+1,solution[i]);
    }
    fclose(f_solution);
    //记录运行数据
    FILE *f_run_data=NULL;
    if((f_run_data=fopen(filename_run_data,"a"))==NULL){
        printf("fopen run data error!");
        exit(-1);
    }
    //记录相关数据，其中最后一项是程序开始运行的年月日时分
    fprintf(f_run_data,"%d %s %d %.3f %d %04d%02d%02d%02d%02d\n",THE_CHOICE,instances[THE_CHOICE],num_color,run_time,generation,1900+p_time_struct->tm_year,1+p_time_struct->tm_mon,p_time_struct->tm_mday,p_time_struct->tm_hour,p_time_struct->tm_min);//类型(choice)、文件名、颜色数、运行时间、交叉次数、日期时间
    fclose(f_run_data);
}
//对解进行检查，输入解，输出结果
void check_answer(int *solution)
{
    int i;
    int j;
    int flag=1;
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            if(solution[i]==solution[t_adjacent_half[i][j+1]]){
                printf("%d,%d should not have the same color!\n",i,t_adjacent_half[i][j+1]);
                flag=0;
                printf("\a");
            }
        }
    }
}
//收尾处理，包括销毁变量以便循环测试
void free_variable(void)
{
    free(filename_in);
    free(filename_out);
    free(filename_run_data);

    int i;
    int j;
    for(i=0;i<num_node;i++){
        free(t_adjacent_half[i]);
        free(t_adjacent_all[i]);
        free(t_conflict_table[i]);
        free(t_tabu_tenure[i]);
    }
    free(t_adjacent_half);
    free(t_adjacent_all);
    free(t_conflict_table);
    free(t_tabu_tenure);
    for(i=0;i<PARENTS_NUM;i++){
        for(j=0;j<num_color;j++){
            free(parents[i][j]);
        }
        free(parents[i]);
        free(children[i]);
    }
    free(best_solution);
    free(history_solution);
}
/***********************************************/
int main()
{
    int i;
    int j;
    for(i=0;i<TEST_NUM;i++){
        initial_program();
        read_file();
        initial_populaion();
        while(1){
            if(best_conflict<=0){
                break;
            }
            for(j=0;j<PARENTS_NUM;j++){
                crossover(parents,j,children[j]);
            }
            updating(generation%BIG_CICLE_NUM+1);
            generation++;
        }
        check_answer(best_solution);
        write_file(best_solution);

        free_variable();
        printf("%%%.1f\n",100.0*(i+1)/TEST_NUM);
    }
    printf("\a\a\a\a\a");
    return 0;
}
