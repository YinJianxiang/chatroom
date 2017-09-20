/*************************************************************************
	> File Name: server.cpp
	> Author: YinJianxiang
	> Mail: YinJianxiang123@gmail.com
	> Created Time: 2017年08月29日 星期二 14时48分03秒
 ************************************************************************/

#include <mysql/mysql.h>    
#include <stdio.h>  
#include <pthread.h>  
#include <sys/socket.h>  
#include <fcntl.h>  
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <string.h>  
#include <signal.h>  
#include <time.h>  
#include <math.h>  
  
using namespace std;  
  
  
#define LOGIN                    1  
#define REGISTER                 2  
#define SHOW_FRIEND              3  
#define FRIEND_ADD               4  
#define FRIEND_DEL               5  
#define SHOW_GROUP               6    
#define GROUP_CREATE             7  
#define GROUP_JOIN               8  
#define GROUP_QUIT               9  
#define GROUP_DEL                10  
#define CHAT_ONE                 11  
#define CHAT_MANY                12  
#define FILE_SEND_BEGIN          13  
#define FILE_SEND_BEGIN_RP       14  
#define FILE_SEND_STOP_RP        15  
#define FILE_RECV_RE             16  
#define FILE_SEND                17  
#define FILE_RECV_BEGIN          18   
#define FILE_RECV_BEGIN_RP       19  
#define FILE_RECV_STOP_RP        20  
#define FILE_RECV                21  
#define FILE_FINI_RP             22  
#define MES_RECORD               23 
#define DEAL_FRIEND              24  
#define FRIEND_ACCEPT            25  
#define EXIT                     -1  
  
  
  
  
#define FILE_STATUS_RECV_ING       1  
#define FILE_STATUS_RECV_STOP      2  
#define FILE_STATUS_SEND_ING       3  
#define FILE_STATUS_SEND_FINI      4  
  
  
#define SIZE_PASS_NAME  30  
#define MAX_CHAR        1024  
#define EPOLL_MAX       200000  
#define LISTENMAX       1000  
#define USER_MAX        100  //the user on line  
#define GROUP_MAX       50  
#define FILE_MAX        50  
#define NUM_MAX_DIGIT   10  
  
#define DOWNLINE   0  
#define ONLINE     1  
#define BUZY       2  
  
   
typedef struct infor_user {  
    char username[MAX_CHAR];  
    char password[20];  
    int  status;       
    int  socket_id;  
    char friends[USER_MAX][MAX_CHAR]; 
    int  friends_num;  
    char group[GROUP_MAX][MAX_CHAR];   
    char group_num;  
}INFO_USER;  
  
  
INFO_USER  user_info  [USER_MAX];  
int        user_num;   
  
  

   
typedef struct infor_group {  
    char  group_name[MAX_CHAR];  
    int   member_num;  
    char  member_name[USER_MAX][MAX_CHAR];   
}INFO_GROUP;  
  
INFO_GROUP   group_info  [GROUP_MAX];  
int          group_num;  


 
  
typedef struct file_infor {  
    char  file_name[MAX_CHAR];  
    char  file_send_name[MAX_CHAR];  
    char  file_recv_name[MAX_CHAR];  
    int   file_size;  
    int   file_size_now;  
    int   flag ;  
}INFO_FILE;  
  
INFO_FILE    file_info  [FILE_MAX]; //begin from 1  
int          file_num;  
  
  

  
typedef struct data {  
    char     send_name[MAX_CHAR];  
    char     recv_name[MAX_CHAR];  
    int      send_fd;  
    int      recv_fd;  
    time_t   time;  
    char     mes[MAX_CHAR*2];  
}DATA;  
  
typedef struct package{  
    int   type;  
    DATA  data;  
}PACK;  
  
  
  
PACK send_pack_info   [MAX_CHAR*2];  
int  send_pack_num;  
  
  
  
typedef struct pthread_parameter  {  
    int a;  
    char str1[SIZE_PASS_NAME];  
    char str2[SIZE_PASS_NAME];  
}PTHREAD_PAR;  
  
void my_err(const char * err_string,int line);  
void signal_close(int i);  
void send_pack(PACK *pack_send_pack_t);      
int  find_userinfor(char username_t[]);  
void login(PACK *recv_pack);  
int  judge_usename_same(char username_t[]);  
void registe(PACK *recv_pack);  
void send_status(PACK *recv_pack);  
void friend_add(PACK *recv_pack); 
void friend_apply(PACK *recv_pack); 
void del_friend_infor(int id,char friend_name[]);   
void friend_del(PACK *recv_pack);  
void chat_with_one(PACK *recv_pack);  
void group_create(PACK *recv_pack);  
void group_join(PACK *recv_pack);  
void del_group_from_user(char *username,char *groupname);  
void group_quit(PACK *recv_pack);  
void group_del_one(int id);  
void group_del(PACK *recv_pack);  
int  find_groupinfor(char groupname_t[]);  
void chat_with_group(PACK *recv_pack);  
int  find_fileinfor(char *filename);  
void file_recv_begin(PACK *recv_pack);  
void file_recv(PACK *recv_pack);  
void *pthread_check_file(void *arg);  
void send_pack_memcpy_server(int type,char *send_name,char *recv_name,int sockfd1,char *mes);  
void *file_send_send(void *file_send_begin_t);  
void file_send_begin(PACK *recv_pack);  
void file_send_finish(PACK *recv_pack);  
void *deal(void *recv_pack_t);  
void *server_send_thread(void *arg);  
void init_server_pthread();  
int  read_infor();  
int  write_infor();  
  
int listenfd,epollfd; 
char* IP = "127.0.0.1";//服务器IP  
short PORT = 10222;//端口号   
int log_file_fd;  
int user_infor_fd;  
int group_infor_fd;  
pthread_mutex_t  mutex;  
pthread_mutex_t  mutex_recv_file;    
pthread_mutex_t  mutex_check_file;


MYSQL           mysql;  
MYSQL_RES       *res = NULL;  
MYSQL_ROW       row;  
char            query_str[MAX_CHAR*4] ;  
int             rc, fields;  
int             rows;  
  

  
  
void my_err(const char * err_string,int line) {  
    fprintf(stderr, "line:%d  ", line);  
    perror(err_string);  
    exit(1);  
}  
  
  
  
  
void signal_close(int i)  {  
    //关闭服务器前 关闭服务器的socket  
    close(log_file_fd);  
      
    write_infor();   
    mysql_free_result(res);  
    mysql_close(&mysql);  
    printf("服务器已经关闭\n");  
    exit(1);  
}  
  
  
  
void send_pack(PACK *pack_send_pack_t) {  
    pthread_mutex_lock(&mutex);    
    memcpy(&(send_pack_info[send_pack_num++]),pack_send_pack_t,sizeof(PACK));  
    pthread_mutex_unlock(&mutex);    
}  
  
  
int read_infor()  {  
    INFO_USER read_file_t;  
    if((user_infor_fd = open("user_infor.txt",O_RDONLY | O_CREAT , S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return -1;  
    }   
    int length;  
    while((length = read(user_infor_fd, &read_file_t, sizeof(INFO_USER))) > 0) {  
        read_file_t.status = DOWNLINE;  
        user_info[++user_num]  = read_file_t;  
    }   
    close(user_infor_fd);  
      
  
    INFO_GROUP read_file_group;  
  
    if((group_infor_fd = open("group_infor.txt",O_RDONLY | O_CREAT , S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return -1;  
    }   
  
    while((length = read(group_infor_fd, &read_file_group, sizeof(INFO_GROUP))) > 0) {  
        group_info[++group_num]  = read_file_group;  
    }   
    close(group_infor_fd);  
    
}  
  
  
  
int write_infor() {  
    INFO_USER read_file_t;  
    if((user_infor_fd = open("user_infor.txt",O_WRONLY | O_CREAT |O_TRUNC , S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return -1;  
    }   
    int length;  
    for(int i = 1 ; i<=user_num ;i++) {  
        if(write(user_infor_fd, user_info+i, sizeof(INFO_USER)) < 0) {  
            my_err("close",__LINE__);  
            return -1;  
        }  
    }  
    close(user_infor_fd);  
      
  
  
    if((group_infor_fd = open("group_infor.txt",O_WRONLY | O_CREAT |O_TRUNC , S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return -1;  
    }   
    for(int i = 1 ; i <= group_num ;i++) {  
        if(write(group_infor_fd, group_info + i, sizeof(INFO_GROUP)) < 0) {  
            my_err("close",__LINE__);  
            return -1;  
        }  
    }  
  
    close(group_infor_fd);   
}  
  
  
 
  
 
int conect_mysql_init() {  
    if (NULL == mysql_init(&mysql)) {               //初始化mysql变量  
        printf("mysql_init(): %s\n", mysql_error(&mysql));  
        return -1;  
    }  
  
    if (NULL == mysql_real_connect(&mysql,         //链接mysql数据库  
                "localhost",                       //链接的当地名字  
                "root",                            //用户名字  
                "251024yin",                            //用户密码  
                "chatroom",                          //所要链接的数据库  
                0,  
                NULL,  
                0)) {  
        printf("mysql_real_connect(): %s\n", mysql_error(&mysql));  
        return -1;  
    }  
    printf("Connected MySQL successful! \n");    
}  
 
 
int find_userinfor(char username_t[])  {  
    int i;  
    if(user_num == 0)  
        return 0;  
    for(i = 1;i <= user_num;i++) {  
        if(strcmp(user_info[i].username,username_t) == 0)  
            return i;  
    }  
    if(i == user_num+1)   
        return 0;  
}  
  
 
  
void login(PACK *recv_pack) {  
    int id = 0;  
    char login_flag[10];  
    
    
    for(int i = 1;i <= user_num;i++) {
        printf("id :%d %s %s %d\n",i,user_info[i].username,user_info[i].password,user_info[i].status);
    }
    

    if((id = find_userinfor(recv_pack->data.send_name)) == 0){ //not exit username  
        login_flag[0] = '2';  
    } else if (user_info[id].status == ONLINE) {  
        login_flag[0] = '3';  
    } else if(strcmp(user_info[id].password,recv_pack->data.mes) == 0){ //the password is crrect  
        login_flag[0] = '1';  
          
        user_info[id].socket_id = recv_pack->data.send_fd;  
    } else { 
        login_flag[0] = '0';  
    }
    printf("login_flag:%c\n",login_flag[0]);  
    login_flag[1] = '\0';  
      
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
    strcpy(recv_pack->data.mes,login_flag);  
    recv_pack->data.recv_fd = recv_pack->data.send_fd;  
    recv_pack->data.send_fd = listenfd;  
    send_pack(recv_pack);  
    usleep(10);  
    if(login_flag[0] == '1')  
        user_info[id].status = ONLINE;  
    free(recv_pack);  
}  

int judge_usename_same(char username_t[]) {  
    int i;  
    if(user_num == 0)  
        return 1;  
    for(i = 1;i <= user_num;i++) {  
        if(strcmp(user_info[i].username,username_t) == 0)  
            return 0;  
    }  
    if(i == user_num + 1)   
        return 1;  
}  
  
  

void registe(PACK *recv_pack) {  
    char registe_flag[10];  
      
    if(judge_usename_same(recv_pack->data.send_name)){
        registe_flag[0] = '1';  

        user_num++;  
        strcpy(user_info[user_num].username,recv_pack->data.send_name);  
        strcpy(user_info[user_num].password,recv_pack->data.mes);  
        printf("\n\n\033[;44m&&&&function registe&&&&\033[0m \n");  
        printf("\033[;44m*\033[0m registe success!\n");  
        printf("\033[;44m*\033[0m username:%s\n", user_info[user_num].username);  
        printf("\033[;44m*\033[0m password:%s\n", user_info[user_num].password);  
        printf("\033[;44m*\033[0m user_num:%d\n\n", user_num);  
        user_info[user_num].status = DOWNLINE;  
    }  
    else registe_flag[0] = '0';  
    registe_flag[1] = '\0';  
     
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
    strcpy(recv_pack->data.mes,registe_flag);  
    recv_pack->data.recv_fd = recv_pack->data.send_fd;  
    recv_pack->data.send_fd = listenfd;  
    send_pack(recv_pack);  
    free(recv_pack);     
}  
 
void send_status(PACK *recv_pack) {  
    char str[MAX_CHAR];  
    char name_t[MAX_CHAR];  
    char send_status_mes[MAX_CHAR*2];  
    int id;  
    int count = 0;  
  
    memset(send_status_mes,0,sizeof(send_status_mes));  
      
    id = find_userinfor(recv_pack->data.send_name);  
   
    send_status_mes[count++] = user_info[id].friends_num;  
  
  
    for(int i = 1 ;i <= user_info[id].friends_num ;i++){  
        strcpy(name_t,user_info[id].friends[i]);  
        printf("send_status friend:%d %s\n",i,name_t);  
        for(int j = 1 ;j <= user_num ;j++) {  
            if(strcmp(name_t,user_info[j].username) == 0) {  
                memset(str,0,sizeof(str));  
                if(user_info[j].status == ONLINE)  
                    sprintf(str,"%d%s\0",ONLINE,user_info[j].username);  
                else  
                    sprintf(str,"%d%s\0",DOWNLINE,user_info[j].username);  
                //printf("str = %s\n",str);  
                for(int k = 0 ;k < SIZE_PASS_NAME;k++) {  
                    send_status_mes[k+count] = str[k];  
                }  
                count += SIZE_PASS_NAME;  
            }  
        }  
    }  
  
    send_status_mes[count++] = user_info[id].group_num;  
    for(int i = 1;i <= user_info[id].group_num;i++) {  
        memset(str,0,sizeof(str));  
        strcpy(name_t,user_info[id].group[i]);  
        sprintf(str,"%s\0",name_t);  
        for(int k = 0 ;k< SIZE_PASS_NAME;k++) {  
            send_status_mes[k+count] = str[k];  
        }  
        count += SIZE_PASS_NAME;  
    }  
  
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
    memcpy(recv_pack->data.mes,send_status_mes,MAX_CHAR*2);  
    recv_pack->data.recv_fd = recv_pack->data.send_fd;  
    recv_pack->data.send_fd = listenfd;  
  
    send_pack(recv_pack);  
    free(recv_pack);  
}  

void friend_apply(PACK *recv_pack) { //给林一个人发送
    char name[MAX_CHAR];
    char buf[MAX_CHAR*2];

    strcpy(name,recv_pack->data.mes);

    int t;
    t = judge_usename_same(name);
    if(t) {
        perror("find_userinfo");
        return;
    }
    int recv_id;
    recv_id = user_info[t].socket_id;
    memset(buf,0,sizeof(buf));
    printf("---%s\n",recv_pack->data.send_name);

    strcpy(buf,recv_pack->data.send_name);
    /*
    for(int i = 0;i < SIZE_PASS_NAME;i++) {
        buf[i] = recv_pack->data.send_name[i];
    }
    */
    recv_pack->type = DEAL_FRIEND;
    strcpy(recv_pack->data.recv_name,name);
    strcpy(recv_pack->data.send_name,"server");
    strcpy(recv_pack->data.mes,buf);
    recv_pack->data.recv_fd = recv_id;
    recv_pack->data.send_fd = listenfd;
    send_pack(recv_pack);
    printf("发给另一个人包了\n");
    free(recv_pack);
}

  
void friend_add(PACK *recv_pack) {  //接收另一个人的决定
    int id_own,id_friend;  
    id_own = find_userinfor(recv_pack->data.send_name);  
        
        
    strcpy(user_info[id_own].friends[++(user_info[id_own].friends_num)],recv_pack->data.mes);  
        
    id_friend = find_userinfor(recv_pack->data.mes);  
    strcpy(user_info[id_friend].friends[++(user_info[id_friend].friends_num)],recv_pack->data.send_name);  
        
    //printf("friend add  user_info[id].friends_num:%d\n", user_info[id].friends_num);  
    //printf("friend add  user_info[id].friends_num:%s\n", user_info[id].friends[user_info[id].friends_num]);  
    
    free(recv_pack);  
}  
  
void del_friend_infor(int id,char friend_name[])  {  
    int id_1;  
    for(int i = 1 ;i <= user_info[id].friends_num;i++)  {  
        if(strcmp(user_info[id].friends[i],friend_name) == 0) {     
            id_1 = i;  
            break;  
        }  
  
    }  
    for(int i = id_1;i < user_info[id].friends_num;i++)  {  
        strcpy(user_info[id].friends[i],user_info[id].friends[i+1]);  
    }  
    user_info[id].friends_num--;  
}  
   
  
  
  
  
  
void friend_del(PACK *recv_pack)  {  
    int id_own,id_own_1;  
    int id_friend,id_friend_1;  
    id_own = find_userinfor(recv_pack->data.send_name);  
        
    del_friend_infor(id_own,recv_pack->data.mes);   
    
    id_friend = find_userinfor(recv_pack->data.mes);  
  
    del_friend_infor(id_friend,recv_pack->data.send_name);   
    //printf("friend add  user_info[id].friends_num:%d\n", user_info[id].friends_num);  
    //printf("friend add  user_info[id].friends_num:%s\n", user_info[id].friends[user_info[id].friends_num]);  
    free(recv_pack);  
}  
  
  
void chat_with_one(PACK *recv_pack)  {  
    sprintf(query_str, "insert into message_tbl values ('%s','%s' ,'%s','%s')",recv_pack->data.send_name,recv_pack->data.recv_name,recv_pack->data.mes,ctime(&recv_pack->data.time));  
    rc = mysql_real_query(&mysql, query_str, strlen(query_str));    
    if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
        return ;  
    }  
    printf("message get into the mysql!!\n");  
    
    send_pack(recv_pack);  
    free(recv_pack);  
}  
  
  
  
  
void group_create(PACK *recv_pack)  {  
    for(int i = 1;i <= group_num;i++)  {  
        if(strcmp(group_info[i].group_name,recv_pack->data.mes) == 0)  {  
            strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
            strcpy(recv_pack->data.send_name,"server");  
            recv_pack->data.mes[0] = 1;  
            send_pack(recv_pack);  
            free(recv_pack);  
            return ;  
        }     
    }  
      
    strcpy(group_info[++group_num].group_name,recv_pack->data.mes);  
    strcpy(group_info[group_num].member_name[++group_info[group_num].member_num],recv_pack->data.send_name);  
    int id = find_userinfor(recv_pack->data.send_name);  
    strcpy(user_info[id].group[++user_info[id].group_num],recv_pack->data.mes);  
  
  
    printf("\n\n\033[;32mcreat group : %s  successfully!\033[0m \n\n", recv_pack->data.mes);  
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
  
    recv_pack->data.mes[0] = 2;  
    send_pack(recv_pack);  
    free(recv_pack);  
}  
  
  
void group_join(PACK *recv_pack)  {  
    for(int i = 1;i <= group_num;i++)  {  
        if(strcmp(group_info[i].group_name,recv_pack->data.mes) == 0)  {  
            strcpy(group_info[i].member_name[++group_info[i].member_num],recv_pack->data.send_name);  
            int id = find_userinfor(recv_pack->data.send_name);  
            strcpy(user_info[id].group[++user_info[id].group_num],recv_pack->data.mes);  
  
            strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
            strcpy(recv_pack->data.send_name,"server");  
            recv_pack->data.mes[0] = 2;   
            printf("\n\n\033[;32m %s join group : %s  successfully!\033[0m \n\n",recv_pack->data.recv_name, recv_pack->data.mes);    
            send_pack(recv_pack);  
            free(recv_pack);  
            return ;  
        }     
    }  
      
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
  
    recv_pack->data.mes[0] = 1;  
    send_pack(recv_pack);  
    free(recv_pack);  
}  

void del_group_from_user(char *username,char *groupname) {  
    int id = find_userinfor(username);  
    for(int i = 1;i<=user_info[id].group_num;i++)  {  
        if(strcmp(user_info[id].group[i],groupname) == 0) {  
            for(int j = i;j < user_info[id].group_num ;j++) {  
                strcpy(user_info[id].group[j],user_info[id].group[j+1]);  
            }  
            user_info[id].group_num--;  
        }  
    }  
}  
  
void group_quit(PACK *recv_pack)  {  
    del_group_from_user(recv_pack->data.send_name,recv_pack->data.mes);  
    for(int i = 1 ;i <= group_num;i++) {  
        if (strcmp(recv_pack->data.mes,group_info[i].group_name) == 0) {  
            for(int j = 1;j <= group_info[i].member_num;j++) {  
                if(strcmp(recv_pack->data.send_name,group_info[i].member_name[j]) == 0) {  
                    for(int k = j;k <group_info[i].member_num;k++) {  
                        strcpy(group_info[i].member_name[k],group_info[i].member_name[k+1]);  
                    }  
                    group_info[i].member_num--;    
                }  
            }  
        }  
    }  
}  
  
void group_del_one(int id) {  
    for(int i = 1;i <= group_info[id].member_num;i++) {  
        del_group_from_user(group_info[id].member_name[i],group_info[id].group_name);  
    }  
  
    for(int i = id ;i < group_num;i++) {  
        group_info[i] = group_info[i+1];  
    }  
    group_num--;  
}  
  
void group_del(PACK *recv_pack) {  
    for(int i = 1;i <= group_num;i++)  {  
        if(strcmp(group_info[i].group_name,recv_pack->data.mes) == 0) {  
            if(strcmp(group_info[i].member_name[1],recv_pack->data.send_name) == 0) {  
                group_del_one(i);  
                recv_pack->data.mes[0] = 2;  
                printf("\n\n\033[;32m delete group : %s  successfully!\033[0m \n\n",recv_pack->data.mes);  
            }  
            else  
                recv_pack->data.mes[0] = 1;  
        }  
    }  
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
    strcpy(recv_pack->data.send_name,"server");  
    send_pack(recv_pack);  

    free(recv_pack);  
  
}  
  
int find_groupinfor(char groupname_t[]) {  
    int i;  
    if(group_num == 0)  
        return 0;  
    for(i = 1;i <= group_num;i++) {  
        if(strcmp(group_info[i].group_name,groupname_t) == 0)  
            return i;  
    }  
    if(i == group_num + 1)   
        return 0;  
}  
/*
void show_group_mem(PACK *recv_pack) {
    int id = find_groupinfor(recv_pack->data.recv_name);
    int t = 0;

    for(int i = 1; i <= group_info[id].member_num;i++)  {  
        printf("%s\n",group_info[id].member_name[i]); 
        for(int j = 0;j < SIZE_PASS_NAME;j++) {
            recv_pack->data.mes[t++] = group_info[id].member_name[i][j];
        }
    }
    send_pack(recv_pack);
    free(recv_pack);
}
*/


void chat_with_group(PACK *recv_pack)  {  
    int id = find_groupinfor(recv_pack->data.recv_name);  
    int len_mes = strlen(recv_pack->data.mes);  
    char send_name[SIZE_PASS_NAME];  
  
    for(int i = len_mes;i >= 0;i--){  
        recv_pack->data.mes[i+SIZE_PASS_NAME] = recv_pack->data.mes[i];  
    }  
      
    strcpy(send_name,recv_pack->data.send_name);  
    for(int i = 0;i < SIZE_PASS_NAME;i++){  
        recv_pack->data.mes[i] = recv_pack->data.send_name[i];  
    }  
  
  
    strcpy(recv_pack->data.send_name,recv_pack->data.recv_name);  
  
  
    for(int i = 1; i <= group_info[id].member_num;i++)  {  
        strcpy(recv_pack->data.recv_name,group_info[id].member_name[i]);  
        if(strcmp(send_name , group_info[id].member_name[i]) != 0) {  
            sprintf(query_str, "insert into message_tbl values ('%s','%s' ,'%s','%s')",recv_pack->data.send_name,recv_pack->data.recv_name,recv_pack->data.mes,ctime(&recv_pack->data.time));  
            rc = mysql_real_query(&mysql, query_str, strlen(query_str));     //对数据库执行 SQL 语句  
            if (0 != rc) {  
                printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
                return ;  
            }   
            
            send_pack(recv_pack);  
        }  
    }  
      
    free(recv_pack);  
}  
  
  
 

  
int find_fileinfor(char *filename) {  
    for(int i = 1;i <= file_num ;i++) {  
        if(strcmp(filename , file_info[i].file_name) == 0) {  
            return i;  
        }  
    }  
    return 0;  
}  
  

  
//根据包的内容，客户端请求发送文件
//首先根据是否有该文件信息
// 有则返回文件的大小
//否则创建文件并返回0
void file_recv_begin(PACK *recv_pack) {  
    int flag = 0;  
    int i;  
    int file_size_now_t;  
  
    pthread_mutex_lock(&mutex_check_file);    
      
    for(i = 1 ;i <= file_num ;i++) {  
        if(strcmp(file_info[i].file_name,recv_pack->data.mes + NUM_MAX_DIGIT) == 0) {  //文件存在
            file_size_now_t = file_info[i].file_size_now;  
            flag = 1;  
            break;  
        }   
    }  
      
    if(!flag) {  
        file_size_now_t = 0;  
        strcpy(file_info[++file_num].file_name,recv_pack->data.mes+NUM_MAX_DIGIT);  
        strcpy(file_info[file_num].file_send_name,recv_pack->data.send_name);  
        strcpy(file_info[file_num].file_recv_name,recv_pack->data.recv_name);  
          
        int t = 0;  
        for(int k = 0;k < NUM_MAX_DIGIT;k++) {  
            if(recv_pack->data.mes[k] == -1)  
                break;  
            int t1 = 1;  
            for(int l = 0 ;l < k;l++)  
                t1 *= 10;  
            t += (int)(recv_pack->data.mes[k])*t1;  
        }
        //求文件的长度  
        //printf("file_recv_begin t :%d\n", t); 
         
        //文件信息
        file_info[file_num].file_size = t;  
        file_info[file_num].file_size_now  = 0;  
        file_info[file_num].flag = FILE_STATUS_RECV_ING;  
    }  
  
    pthread_mutex_unlock(&mutex_check_file);    
  
    recv_pack->type = FILE_SEND_BEGIN_RP;  
    strcpy(recv_pack->data.recv_name,recv_pack->data.send_name);  
      
  
    int digit = 0;  
    while(file_size_now_t != 0) {     
        recv_pack->data.mes[digit++] = file_size_now_t % 10;  
        file_size_now_t /= 10;  
    }  
    recv_pack->data.mes[digit]  = -1;  
  
    send_pack(recv_pack);  
    free(recv_pack);  
}  
  
//包中的内容写入文件中
void file_recv(PACK *recv_pack) {  
    pthread_mutex_lock(&mutex_recv_file);    
    int fd;  
    char file_name[MAX_CHAR];  
    char file_path_t[SIZE_PASS_NAME];  
      
  
    int len = 0;  
    for(int i = 0 ;i < NUM_MAX_DIGIT ;i++) {  
        if(recv_pack->data.mes[i] == -1)    
            break;  
        int t1 = 1;  
        for(int l = 0;l < i;l++)  
            t1 *= 10;  
        len += (int)recv_pack->data.mes[i]*t1;  
    }  
  
    //printf("\033[;33mlen = %d\033[0m \n", len);  
      
    strcpy(file_name,recv_pack->data.recv_name);  
    //you can creat this file when you get the file_send_begin  
    if((fd = open(file_name,O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return ;  
    }  
  
    if(write(fd,recv_pack->data.mes + NUM_MAX_DIGIT,len) < 0)  
        my_err("write",__LINE__);  
    // 关闭文件   
    close(fd);  
      
    printf("file_name :%s\n", file_name);  
      
    int id  =  find_fileinfor(file_name);  
      
    file_info[id].file_size_now += len;  
      
    free(recv_pack);  
  
    pthread_mutex_unlock(&mutex_recv_file);    
}  
  
  
//检查文件的状态，失败就提醒客户端  
void *pthread_check_file(void *arg) {  
    while(1) {  
        pthread_mutex_lock(&mutex_check_file);    
        for(int i = 1 ;i <= file_num ;i++)  {  
            if(file_info[i].file_size <= file_info[i].file_size_now && (file_info[i].flag == FILE_STATUS_RECV_ING ||file_info[i].flag == FILE_STATUS_RECV_STOP)) {  
                char mes_t[MAX_CHAR]; 
                int id = find_userinfor(file_info[i].file_recv_name);  
  
                PACK *pthread_check_file_t = (PACK*)malloc(sizeof(PACK));  
                memset(pthread_check_file_t, 0,sizeof(PACK));  
                pthread_check_file_t->type = FILE_RECV_BEGIN;  
                strcpy(pthread_check_file_t->data.send_name ,file_info[i].file_send_name);  
                strcpy(pthread_check_file_t->data.recv_name,file_info[i].file_recv_name);  
                  
                int len = file_info[i].file_size;  
  
                memset(mes_t,0,sizeof(mes_t));  
                int digit = 0;  
                while(len != 0) {     
                    mes_t[digit++] = len%10;  
                    len /= 10;  
                }  
                mes_t[digit]  = -1;  
                for(int j = 0 ;j< SIZE_PASS_NAME ;j++) {  
                    mes_t[NUM_MAX_DIGIT+j] = file_info[i].file_name[j];  
                }    
  
  
                memcpy(pthread_check_file_t->data.mes,mes_t,sizeof(mes_t));    
                send_pack(pthread_check_file_t);  
                free(pthread_check_file_t);  
                file_info[i].flag = FILE_STATUS_SEND_ING;  
  
                break;  
            }  
  
  
            if(file_info[i].file_size > file_info[i].file_size_now ) {  
                int id = find_userinfor(file_info[i].file_send_name);  
                if(user_info[id].status == DOWNLINE && file_info[i].flag == FILE_STATUS_RECV_ING) {  
                    printf("\033[;35m file_name %s d\033[0m\n", file_info[i].file_name);  
                      
                    char mes[MAX_CHAR];  
                    memset(mes,0,sizeof(mes));  
                    int num = file_info[i].file_size_now;  
                      
                    printf("\033[;35m file_sizejj :%d\033[0m\n",num );  
  
                    int digit = 0;  
                    while(num != 0) {     
                        mes[digit++] = num%10;  
                        num /= 10;  
                    }  
                    mes[digit] = -1;  
                    for(int i = 0;i < 10;i++)  
                        printf("%d\n\n",mes[i]);  
  
                    printf("\033[;35m file_name %s \033[0m\n", file_info[i].file_name);  
                     
  
                    PACK pthread_check_file_t;  
                    memset(&pthread_check_file_t, 0,sizeof(PACK));  
                    pthread_check_file_t.type = FILE_SEND_STOP_RP;  
                    strcpy(pthread_check_file_t.data.send_name ,file_info[i].file_name);  
                    strcpy(pthread_check_file_t.data.recv_name,file_info[i].file_send_name);  
                    memcpy(pthread_check_file_t.data.mes,mes,sizeof(mes));  
                      
                    send_pack(&pthread_check_file_t);  
 
                    file_info[i].flag = FILE_STATUS_RECV_STOP;  
                    break;  
                }  
            }  
            if(file_info[i].flag == FILE_STATUS_SEND_FINI) {  
                unlink(file_info[i].file_name);  
                file_num --;  
                for(int j = i;j<=(file_num+1)&&file_num;j++) {  
                    file_info[j] = file_info[j+1];  
                }   
  
            }       
        }  
        pthread_mutex_unlock(&mutex_check_file);   
        usleep(10);  
    }      
}  
  
//发送文件包函数  
void send_pack_memcpy_server(int type,char *send_name,char *recv_name,int sockfd1,char *mes) {  
    PACK pack_send_pack;  
    time_t timep;  
    pack_send_pack.type = type;  
    strcpy(pack_send_pack.data.send_name,send_name);  
    strcpy(pack_send_pack.data.recv_name,recv_name);  
    memcpy(pack_send_pack.data.mes,mes,MAX_CHAR*2);   
    time(&timep);  
    pack_send_pack.data.time = timep;  
    printf("sockfd1:%d\n", sockfd1);  
    if(send(sockfd1,&pack_send_pack,sizeof(PACK),0) < 0){  
        my_err("send",__LINE__);  
    }  
}  



  
//根据文件的起始位置，开始发送， 
//在发送过程中，不断监测接收端的状态 
//若接收端下线，则向接收端发送提醒消息  
void *file_send_send(void *file_send_begin_t)  {  
    PTHREAD_PAR *file_send_begin = (PTHREAD_PAR *)file_send_begin_t;  
    int fd;  
    int length;  
    int status;  
    int id;  
    int sockfd;  
    int file_size;  
    int begin_location = file_send_begin->a;  
    int sum = begin_location;  
    char file_name[SIZE_PASS_NAME];  
    char recv_name[SIZE_PASS_NAME];  
    char mes[MAX_CHAR*2];  
    printf("\n\nsending the file.........\n");  
      
    strcpy(file_name,file_send_begin->str1);  
    strcpy(recv_name,file_send_begin->str2);  
  
    if((fd = open(file_name,O_RDONLY)) == -1) {  
        my_err("open",__LINE__);  
        return NULL;  
    }  
      
    file_size=lseek(fd, 0, SEEK_END);  
      
    lseek(fd ,begin_location ,SEEK_SET);  
    printf("file_size %d\n", file_size);  
    printf("begin_location%d\n", begin_location);  
    bzero(mes, MAX_CHAR*2);   
  
    // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止   
    while((length = read(fd  ,mes + NUM_MAX_DIGIT ,MAX_CHAR*2 - NUM_MAX_DIGIT)) > 0)  {  
        printf("send_::%d\n", sum);  
  
        if(sum == file_size)    
            break;  
  
        id = find_userinfor(recv_name);  
        if(user_info[id].status == DOWNLINE) {  
            int  file_id   = find_fileinfor(file_name);  
            int  file_size = file_info[file_id].file_size;  
            char mes_t[MAX_CHAR];  
            PACK file_send_stop_t;  
  
            memset(&file_send_stop_t, 0,sizeof(PACK));  
            file_send_stop_t.type = FILE_RECV_STOP_RP;  
            strcpy(file_send_stop_t.data.send_name ,file_info[file_id].file_send_name);  
            strcpy(file_send_stop_t.data.recv_name,file_info[file_id].file_recv_name);  
                  
  
            memset(mes_t,0,sizeof(mes_t)); 


            int digit = 0;  
            while(file_size != 0) {     
                mes_t[digit++] = file_size % 10;  
                file_size /= 10;  
            }  
            mes_t[digit]  = -1;  
            for(int j = 0 ;j < SIZE_PASS_NAME ;j++) {  
                mes_t[NUM_MAX_DIGIT+j] = file_info[file_id].file_name[j];  
            }    
  
  
            memcpy(file_send_stop_t.data.mes,mes_t,sizeof(mes_t));    
            send_pack(&file_send_stop_t);  
  
            free(file_send_begin);  
            return NULL ;  
        }  
  
        sockfd = user_info[id].socket_id;  

        //文件已发送的字节 
        int t = length;  
        int digit = 0;  
        while(t != 0) {     
            mes[digit++] = t%10;  
            t /= 10;  
        }  
        mes[digit]  = -1;  
        send_pack_memcpy_server(FILE_RECV,file_name,recv_name,sockfd,mes);  
        sum += length;  
          
        bzero(mes, MAX_CHAR*2);   
        usleep(100000);  
    }   
    // 关闭文件   
    close(fd);  
    free(file_send_begin);  
}  
  
  
  
  
  
void file_send_begin(PACK *recv_pack) {  
    PTHREAD_PAR *file_send_begin_t;  
    file_send_begin_t = (PTHREAD_PAR *)malloc(sizeof(PTHREAD_PAR));  
    char recv_name[SIZE_PASS_NAME];  
    char file_name[SIZE_PASS_NAME];  
    int  begin_location = 0;  
    strcpy(recv_name,recv_pack->data.send_name);  
    strcpy(file_name,recv_pack->data.recv_name);  
    printf("file name%s\n", file_name);  
      
    //客户端已经接收文件字节数
    for(int i = 0 ;i < NUM_MAX_DIGIT ;i++) {  
        if(recv_pack->data.mes[i] == -1)    
            break;  
        int t1 = 1;  
        for(int l = 0;l < i;l++)  
            t1 *= 10;  
        begin_location += (int)recv_pack->data.mes[i]*t1;  
    }  
    int file_id = find_fileinfor(file_name);  
  
    if(begin_location >= file_info[file_id].file_size) {  
        file_info[file_id].flag  = FILE_STATUS_SEND_FINI;  
    }  
  
    file_send_begin_t->a = begin_location;  
    strcpy(file_send_begin_t->str1,file_name);  
    strcpy(file_send_begin_t->str2,recv_name);  
    file_send_send((void *)file_send_begin_t);  
    free(recv_pack);  
}  
  
//文件发送结束 
void file_send_finish(PACK *recv_pack) {     
    printf("^^^^^^^^^^^^^^^^^^^^^^%s\n",recv_pack->data.mes);  
    int id = find_fileinfor(recv_pack->data.mes);  
    file_info[id].flag = FILE_STATUS_SEND_FINI;  
    free(recv_pack);  
}  
  
  
void send_record(PACK *recv_pack)  {  
    char send_name[MAX_CHAR];  
    char recv_name[MAX_CHAR];  
    char mes[MAX_CHAR*2];  
    PACK *pack_send_record_t = (PACK *)malloc(sizeof(PACK));  
    strcpy(send_name,recv_pack->data.send_name);  
    strcpy(recv_name,recv_pack->data.mes);  
    sprintf(query_str,"select send_name ,recv_name,mes,time from message_tbl where send_name=%s or send_name = %s",send_name,recv_name);            //显示 表中数据  
    rc = mysql_real_query(&mysql, query_str, strlen(query_str));  
    if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
        return ;  
    }  
  
    res = mysql_store_result(&mysql);                             //获取查询结果  
    if (NULL == res) {  
         printf("mysql_restore_result(): %s\n", mysql_error(&mysql));  
         return ;  
    }  
    while ((row = mysql_fetch_row(res)))   {  
        pack_send_record_t->type = MES_RECORD;  
        strcpy(pack_send_record_t->data.send_name,"server");  
        strcpy(pack_send_record_t->data.recv_name,send_name);  
        strcpy(pack_send_record_t->data.mes,row[0]);  //send_name
        strcpy(pack_send_record_t->data.mes + SIZE_PASS_NAME,row[3]);  //mes
        strcpy(pack_send_record_t->data.mes + SIZE_PASS_NAME*2,row[2]);//time  
        send_pack(pack_send_record_t);  
        usleep(10000);  
          
    }  
  
    pack_send_record_t->type = MES_RECORD;  
    strcpy(pack_send_record_t->data.send_name,"server_end");  
    strcpy(pack_send_record_t->data.recv_name,send_name);  
    send_pack(pack_send_record_t);  
  
    usleep(10000);  
    free(pack_send_record_t);  
}  

   
  
void *deal(void *recv_pack_t) {  
    int i;  
    PACK *recv_pack = (PACK *)recv_pack_t;  
    printf("\n\n\ndeal function = %d\n", recv_pack->type);  
    
    switch(recv_pack->type) {  
        case LOGIN:  
            login(recv_pack);  
            break;  
        case REGISTER:  
            registe(recv_pack);  
            break;  
        case SHOW_FRIEND:  
            send_status(recv_pack);  
            break;  
        case FRIEND_ADD: 
            printf("*****%s\n",recv_pack->data.mes);
            friend_add(recv_pack);  
            break;  
        case FRIEND_DEL:  
            friend_del(recv_pack);  
            break;  
        case GROUP_CREATE:  
            group_create(recv_pack);  
            break;  
        case GROUP_JOIN:  
            group_join(recv_pack);  
            break;  
        case GROUP_QUIT:  
            group_quit(recv_pack);  
            break;  
        case GROUP_DEL:  
            group_del(recv_pack);  
            break;  
        case CHAT_ONE:  
            chat_with_one(recv_pack);  
            break;  
        case CHAT_MANY:  
            chat_with_group(recv_pack);  
            break;  
        case FILE_SEND_BEGIN:  
            file_recv_begin(recv_pack);  
            break;  
        case FILE_SEND:  
            file_recv(recv_pack);  
            break;  
        case FILE_SEND_BEGIN_RP:  
            file_send_begin(recv_pack);  
            break;  
        case FILE_FINI_RP:  
            file_send_finish(recv_pack);  
            break;  
        case MES_RECORD:  
            send_record(recv_pack); 
        case DEAL_FRIEND:
            //printf("收到包了\n");
            friend_apply(recv_pack);
            break;
        /*
        case SHOW_GROUP_MEM:
            show_group_mem(recv_pack);
        */
        case EXIT:  
            break;         
    }  
}  
 
  

void *server_send_thread(void *arg)  {  
    int user_status = DOWNLINE;  
    int id_stop;  
    int i,recv_fd_t,recv_fd_online;  
    while(1) {  
        pthread_mutex_lock(&mutex);   
        
        user_status = DOWNLINE;  
        for(i = send_pack_num - 1;i >= 0;i--) {  
            for(int j = 1; j <= user_num ;j++) {  
                if(strcmp(send_pack_info[i].data.recv_name,user_info[j].username) == 0) {  
                    user_status = user_info[j].status;  
                    if(user_status == ONLINE)  
                        recv_fd_online = user_info[j].socket_id;  
                    break;  
                }  
            }  

            //在线或者注册，登录包则发
            if(user_status == ONLINE || send_pack_info[i].type == LOGIN || send_pack_info[i].type == REGISTER) {  
                if(user_status == ONLINE)  
                    recv_fd_t = recv_fd_online;  
                else  
                    recv_fd_t = send_pack_info[i].data.recv_fd;  
                  
  
                if(send(recv_fd_t,&send_pack_info[i],sizeof(PACK),0) < 0){  
                    my_err("send",__LINE__);  
                }  
                printf("\n\n\033[;42m*****send pack****\033[0m\n");  
                printf("\033[;42m*\033[0m type    :%d\n",send_pack_info[i].type);  
                printf("\033[;42m*\033[0m from    : %s\n",send_pack_info[i].data.send_name);  
                printf("\033[;42m*\033[0m to      : %s\n",send_pack_info[i].data.recv_name);  
                printf("\033[;42m*\033[0m mes     : %s\n",send_pack_info[i].data.mes);  
                printf("\033[;42m*\033[0m recv_fd : %d\n",send_pack_info[i].data.recv_fd);  
                send_pack_num--;  
                printf("\033[;42m*\033[0m pack left Now is:%d\n\n", send_pack_num);  
                  
                for(int j = i;j <= send_pack_num && send_pack_num;j++) {  
                    send_pack_info[j] = send_pack_info[j+1];  
                }  
                break;  
            }  
        }  
        pthread_mutex_unlock(&mutex);  
        usleep(1);    
    }  
}  
  
  
void init_server_pthread()  {  
    pthread_t pid_send;
    pthread_t pid_file_check;  
    pthread_create(&pid_send,NULL,server_send_thread,NULL);  
    pthread_create(&pid_file_check,NULL,pthread_check_file,NULL);  
}   
  


int main() {  
    int n;  
      
    pthread_t pid;  
    PACK recv_t;  
    PACK *recv_pack;  
    int err,socketfd,fd_num;  
    struct sockaddr_in fromaddr;  
    socklen_t len = sizeof(fromaddr);  
    struct epoll_event ev, events[LISTENMAX];  
      
    read_infor();  
    /*
    if((log_file_fd = open("server_log.txt",O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return 0;  
    }  
    dup2(log_file_fd,1);//重定向到文件  
    */
    
    conect_mysql_init();  
     
  
  
    signal(SIGINT,signal_close);//退出CTRL+C  
    pthread_mutex_init(&mutex, NULL);    
  
  
    printf("服务器开始启动..\n");  
    init_server_pthread();  
    epollfd = epoll_create(EPOLL_MAX);//生成epoll句柄  
      
    listenfd = socket(AF_INET,SOCK_STREAM,0);//启动socket  
    if(listenfd == -1){  
        perror("创建socket失败");  
        printf("服务器启动失败\n");  
        exit(-1);  
    }  
  
    err = 1;  
  
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&err,sizeof(int));   
  
    ev.data.fd = listenfd;//设置与要处理事件相关的文件描述符  
    ev.events = EPOLLIN;//设置要处理的事件类型  
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);//注册epoll事件  
  
    //准备网络通信地址  
    struct sockaddr_in addr;  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(PORT);  
    addr.sin_addr.s_addr = inet_addr(IP);  
    if(bind(listenfd,(struct sockaddr*)&addr,sizeof(addr))==-1){//绑定服务器  
        perror("绑定失败");  
        printf("服务器启动失败\n");  
        exit(-1);  
    }  
    printf("成功绑定\n");  
  
    //设置监听  
    if(listen(listenfd,10)==-1){  
        perror("设置监听失败");  
        printf("服务器启动失败\n");  
        exit(-1);  
    }  
    printf("设置监听成功\n");  
    printf("初始化服务器成功\n");  
    printf("服务器开始服务\n");  
    while(1) {  
        fd_num = epoll_wait(epollfd, events, EPOLL_MAX, 1000);  
        for(int i = 0; i < fd_num; i++) {  
            if(events[i].data.fd == listenfd)  {     
                socketfd = accept(listenfd,(struct sockaddr*)&fromaddr,&len);  
                printf("%d get conect!\n",socketfd);  
                ev.data.fd = socketfd;  
                ev.events = EPOLLIN;//设置监听事件可写  
                //新增套接字  
                epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &ev);  
            }  
            else if(events[i].events & EPOLLIN) { //fd can read   
                n = recv(events[i].data.fd,&recv_t,sizeof(PACK),0);//读取数据  
                recv_t.data.send_fd = events[i].data.fd;  
                  
  
                if(n < 0) {//recv错误  
                    close(events[i].data.fd);  
                    perror("recv");  
                    continue;  
                } else if(n == 0) { //断开链接 
                    for(int j = 1;j <= user_num;j++)  {  
                        if(events[i].data.fd == user_info[j].socket_id) {  
                            printf("%s down line!\n",user_info[j].username);  
                            user_info[j].status = DOWNLINE;  
                            break;  
                        }  
                    }     
                    ev.data.fd = events[i].data.fd;  
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev); //删除套接字  
                    close(events[i].data.fd);   
                    continue;     
                }  
                pthread_mutex_lock(&mutex);   
                
                pthread_mutex_unlock(&mutex);             
  
                recv_pack = (PACK*)malloc(sizeof(PACK));  
                memcpy(recv_pack, &recv_t, sizeof(PACK));  
                pthread_create(&pid,NULL,deal,(void *)recv_pack);  
            }  
  
        }  
    }  
    return 0;  
}  
  
  
  
  
  
  

