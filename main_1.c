#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define POPULATION_SIZE 10
#define ITERATION_NUM 100000
#define PARENTS_CROSSOVER_NUM 6
/***********************************************/
#define THE_CHOICE 6
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
//t:table   s:solution
int num_color=0;
int num_node=0;
int num_edge=0;
int **t_adjacent_half=NULL;//记录节点的比自身序号小的邻边
int **t_adjacent_all=NULL;//记录节点的所有邻边
int **t_population[POPULATION_SIZE];
int *l_s_best=NULL;
int i_conflict_best;
int *l_conflict_num=NULL;//长度为population_size+1，最后一个位置存子代冲突数
int **t_distance=NULL;//个体间距离表
int *l_min_distance=NULL;//个体与其它个体最小距离列表

int *l_s_curr=NULL;
int **t_conflict_table=NULL;
int **t_tabu_tenure=NULL;
/***********************************************/
//对输入输出的文件名初始化，方便算例的选择，方便操作
void initial_instances_choice(void)
{
    filename_in=(char *)malloc(sizeof(char)*50);
    filename_out=(char *)malloc(sizeof(char)*50);

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
    int record_nontabu[2];//结点，变化后的颜色
    int record_tabu[2];
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
            //curr_conflict_num-=t_conflict_table[t_adjacent_all[temp_node][m+1]][solution[t_adjacent_all[temp_node][m+1]]];
            t_conflict_table[t_adjacent_all[temp_node][m+1]][color_before]--;
            t_conflict_table[t_adjacent_all[temp_node][m+1]][color_after]++;
            //curr_conflict_num+=t_conflict_table[t_adjacent_all[temp_node][m+1]][solution[t_adjacent_all[temp_node][m+1]]];
        }
        t_tabu_tenure[temp_node][color_before]=i+curr_conflict_num+rand()%10;
        if(curr_conflict_num<best_conflict){
            for(m=0;m<num_node;m++){
                best_solution[m]=solution[m];
            }
            best_conflict=curr_conflict_num;
            //printf("iter=%d:Best_f=%d\n",i,best_conflict);
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
        if(record[1]>l_conflict_num[i]){
            record[0]=i;
            record[1]=l_conflict_num[i];
        }
    }
    //记录最优解
    i_conflict_best=record[1];
    for(i=0;i<num_color;i++){
        for(j=0;j<t_population[record[0]][i][0];j++){
            l_s_best[t_population[record[0]][i][j+1]]=i;
        }
    }
//    //初始化距离表
//    int *classes_one=(int *)malloc(sizeof(int)*num_color);
//    int *classes_two=(int *)malloc(sizeof(int)*num_color);
//    int **match_table;
//    for(i=0;i<POPULATION_SIZE-1;i++){
//        for(j=i;j<POPULATION_SIZE;j++){
//
//        }
//    }
}
//动态多父本交叉算子。传入种群、参与交叉的父本数、对应的父本序号
void adaptive_multi_parent_crossover(int ***population,int parent_num,int *parents)
{
    int i;
    int j;
    int m;
    int n;
    int temp_parent_num;
    int temp_parent;
    int temp_node;
    int temp_color;
    int temp_node_num;
    int **population_parents[parent_num];//新建空间存储父本种群
    int *solution_child=(int *)malloc(sizeof(int)*num_node);//产生的子代
    int *solution_parents[parent_num];//父本对应的一维解集
    int *solution_parents_node_position[parent_num];//父本对应的一维解集的结点在二维中的位置
    int max_classes[parent_num][2];//结点数，对应class
    int parents_tabu_table[parent_num];
    for(i=0;i<parent_num;i++){
        max_classes[i][0]=0;
        parents_tabu_table[i]=-1;
        solution_parents[i]=(int *)malloc(sizeof(int)*num_node);
        solution_parents_node_position[i]=(int *)malloc(sizeof(int)*num_node);
    }
    for(i=0;i<parent_num;i++){
        population_parents[i]=(int **)malloc(sizeof(int *)*num_color);
        for(j=0;j<num_color;j++){
            population_parents[i][j]=(int *)malloc(sizeof(int)*num_node);
            temp_parent=parents[i];
            temp_node_num=population[temp_parent][j][0];
            population_parents[i][j][0]=temp_node_num;
            for(m=0;m<temp_node_num;m++){
                temp_node=population[temp_parent][j][m+1];
                population_parents[i][j][m+1]=temp_node;
                solution_parents[i][temp_node]=j;
                solution_parents_node_position[i][temp_node]=m+1;
            }
            if(max_classes[i][0]<population[temp_parent][j][0]){
                max_classes[i][0]=population[temp_parent][j][0];
                max_classes[i][1]=j;
            }
        }
    }
    //交叉算子.i:color
    for(i=0;i<num_color;i++){
        //找到最大子集
        m=-1;
        //temp_parent_num=0;
        for(j=0;j<parent_num;j++){
            if(m<max_classes[j][0]&&i>parents_tabu_table[j]){
                m=max_classes[j][0];
                temp_parent_num=j;
            }
        }
        //将子集复制到子代中,去除父本中用过的结点.
        for(j=0;j<population_parents[temp_parent_num][max_classes[temp_parent_num][1]][0];j++){
            temp_node=population_parents[temp_parent_num][max_classes[temp_parent_num][1]][j+1];
            solution_child[temp_node]=i;
            //对其它父本进行处理，移除该结点
            for(m=1;m<parent_num;m++){
                temp_parent=(temp_parent_num+m)%parent_num;
                temp_color=solution_parents[temp_parent][temp_node];
                for(n=solution_parents_node_position[temp_parent][temp_node];n<population_parents[temp_parent][temp_color][0];n++){
                    population_parents[temp_parent][temp_color][n]=population_parents[temp_parent][temp_color][n+1];
                    solution_parents_node_position[temp_parent][population_parents[temp_parent][temp_color][n+1]]--;
                }
                population_parents[temp_parent][temp_color][0]--;
            }
        }
        population_parents[temp_parent_num][max_classes[temp_parent_num][1]][0]=0;
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
        parents_tabu_table[temp_parent_num]=i+parent_num/2;
    }
    //剩余结点随机分配
    if(max_classes[0][0]>0){
        for(i=0;i<num_color;i++){
            for(j=0;j<population_parents[0][i][0];j++){
                solution_child[population_parents[0][i][j+1]]=rand()%num_color;
            }
        }
    }
    //子代优化
    tabu_search(solution_child,&l_conflict_num[POPULATION_SIZE]);
    //更新种群。先用最简单的方法，根据目标函数大小，用子代替换较差的父本
    temp_parent=parents[0];
    m=l_conflict_num[temp_parent];
    for(i=1;i<parent_num;i++){
        if(m<l_conflict_num[parents[i]]){
            temp_parent=parents[i];
            m=l_conflict_num[temp_parent];
        }
    }
    l_conflict_num[temp_parent]=l_conflict_num[POPULATION_SIZE];
    for(i=0;i<num_color;i++){
        population[temp_parent][i][0]=0;
    }
    for(i=0;i<num_node;i++){
        temp_color=solution_child[i];
        population[temp_parent][temp_color][0]++;
        temp_node=population[temp_parent][temp_color][0];
        population[temp_parent][temp_color][temp_node]=i;
    }
    //更新最优解
    if(i_conflict_best>l_conflict_num[POPULATION_SIZE]){
        i_conflict_best=l_conflict_num[POPULATION_SIZE];
        for(i=0;i<num_node;i++){
            l_s_best[i]=solution_child[i];
        }
        //printf("best_conflict=%d\n",i_conflict_best);
    }
}
//void pool_updating()
//{
//}
//记录结果
void write_file(void)
{
    FILE *f_solution=NULL;
    int i;
    if((f_solution=fopen(filename_out,"w"))==NULL){
        printf("fopen out error!");
        exit(-1);
    }
    for(i=0;i<num_node;i++){
        fprintf(f_solution,"%d %d\n",i+1,l_s_best[i]);
    }
    fclose(f_solution);
    //printf("write answer\n");
}
//对解进行检查，输入解，输出结果
void check_answer(int *solution)
{
    int i;
    int j;
    int flag=1;
    printf("check:");
    for(i=0;i<num_node;i++){
        for(j=0;j<t_adjacent_half[i][0];j++){
            if(solution[i]==solution[t_adjacent_half[i][j+1]]){
                printf("%d,%d should not have the same color!\n",i,t_adjacent_half[i][j+1]);
                flag=0;
                printf("\a");
            }
        }
    }
    if(1==flag){
        printf("the answer is right!\n");
        printf("\a\a\a");
    }
}
/***********************************************/
int main()
{
    int parent_num=2;
    int parents[PARENTS_CROSSOVER_NUM];

    initial_instances_choice();
    read_file();
    initial_populaion();
    while(1){
        if(i_conflict_best==0){
            break;
        }
        parents[0]=rand()%POPULATION_SIZE;
        parents[1]=(rand()%(POPULATION_SIZE-1)+1+parents[0])%POPULATION_SIZE;
        adaptive_multi_parent_crossover(t_population,parent_num,parents);
    }
    check_answer(l_s_best);
    write_file();

    return 0;
}
