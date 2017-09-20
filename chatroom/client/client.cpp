/*************************************************************************
	> File Name: client.cpp
	> Author: YinJianxiang
	> Mail: YinJianxiang123@gmail.com
	> Created Time: 2017年08月29日 星期二 14时48分09秒
 ************************************************************************/

#include <mysql/mysql.h>    
#include <stdio.h>  
#include <pthread.h>  
#include <sys/socket.h>  
#include <sys/epoll.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <string.h>  
#include <signal.h>  
#include <time.h>  
#include <termios.h> 
   

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
  
  
#define SIZE_PASS_NAME   30  
#define MAX_PACK_CONTIAN 100  
#define MAX_CHAR         1024  
#define NUM_MAX_DIGIT    10  
  
#define DOWNLINE   0  
#define ONLINE     1  
  
typedef struct  friend_info {  
    int statu;  
    int mes_num;  
    char name[MAX_CHAR];  
}FRIEND_INFO;   
  
  
  
  
typedef struct user_infor{  
    char        username    [MAX_CHAR];  
    FRIEND_INFO friends     [MAX_CHAR];  
    int         friends_num;  
    char        group       [MAX_CHAR][MAX_CHAR];  
    int         group_num;  
}USER_INFOR;  
  
USER_INFOR now_info;  
  


typedef struct datas{  
    char    send_name[MAX_CHAR];  
    char    recv_name[MAX_CHAR];  
    int     send_fd;  
    int     recv_fd;  
    time_t  time;  
    char    mes[MAX_CHAR*2];  
}DATA;  
  
typedef struct package{  
    int type;  
    DATA  data;  
}PACK;  
  
typedef struct pthread_parameter {  
    int a;  
    int b;  
}PTHREAD_PAR;  
  
  
  
typedef struct prinit_mes {  
    char name[MAX_CHAR];  
    char time[MAX_CHAR];  
    char mes [MAX_CHAR];  
}PRINT_MES;  
  
  
 
  


  
PACK show_friend_pack   [MAX_PACK_CONTIAN];  
PACK chat_pack         [MAX_PACK_CONTIAN];  
PACK send_file_pack    [MAX_PACK_CONTIAN];  
PACK recv_file_mes_pack     [MAX_PACK_CONTIAN];  
PACK recv_pack_file         [MAX_PACK_CONTIAN];  
PACK friend_apply_pack    [MAX_PACK_CONTIAN];
/*

PACK show_group_mem_pack  [MAX_PACK_CONTIAN];

int show_group_mem_pack_num;
*/

int friend_apply_pack_num;
int show_friend_num; 
int recv_chat_num;   
int send_file_num;
int recv_file_mes_num; 
int recv_file_num;
  
int group_create_flag;
int group_join_flag;   
int group_delete_flag;
int print_mes_flag; 
int print_record_flag;
  
PRINT_MES print_mes_info[7];  
int print_mes_num;  

void print_file_mes();  
void my_err(const char * err_string,int line);  
void init();  
void sig_close(int i);  
void send_pack(int type,char *send_name,char *recv_name,char *mes);  
void send_pack_memcpy(int type,char *send_name,char *recv_name,char *mes);  
int  get_choice(char *choice_t);  
int  send_login(char username_t[],char password_t[]);  
int  login();  
int  login_menu();  
int  send_registe(char username_t[],char password_t[]);  
void registe();  
void change_statu(PACK pack_deal_statu_t);  
void *deal_statu(void *arg);  
void send_file_send(int begin_location,char *file_path);  
void *pthread_send_file(void *mes_t);  
void *pthread_recv_file(void *par_t);  
void init_client_pthread();  
void get_status_mes();  
void show_friend();  
int  judge_same_friend(char add_friend_t[]);  
void add_friend(); 
void apply_friend(); 
void delete_friend();  
int  judge_same_group(char *group_name);  
void show_group();  
void send_mes(char mes_recv_name[],int type);  
void print_mes(int id);  
void *show_mes(void *username);  
void chat_with_one();  
void group_create();  
void group_join();  
void group_quit();  
void group_del();  
void chat_with_group();  
int  get_file_size(char *file_name);  
void send_file();  
void file_infor_delete(int id);  
void mes_sendfile_fail(int id);  
void mes_recv_requir(int id);  
void mes_recvfile_fail(int id);  
void deal_file_mes(int id);  
int  file_mes_box();  
void print_mes_record(PACK pack_t);  
int mes_record();  
  
int  main_menu();  
  
  
  
  
int sockfd;  
char *IP = "127.0.0.1";  
short PORT = 10222;  
typedef struct sockaddr SA;  
pthread_mutex_t  mutex_local_user;  
  
pthread_mutex_t  mutex_recv_file;  
  
  
void print_file_mes()  {  
    for(int i = 1 ;i <= 5;i++) {  
        printf("****************\n");  
        printf("%s\n", recv_file_mes_pack[i].data.send_name);  
        printf("%s",recv_file_mes_pack[i].data.recv_name);  
        for(int j=0;j<=5;j++)  
            printf("%d\n\n",recv_file_mes_pack[i].data.mes[j]);  
    }  
}  
  
 
  
void my_err(const char * err_string,int line) {  
    fprintf(stderr, "line:%d  ", line);  
    perror(err_string);  
    exit(1);  
}  
  
void init() {  
    printf("客户端开始启动\n");  
    sockfd = socket(AF_INET,SOCK_STREAM,0);//启动socket  
    struct sockaddr_in addr;  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(PORT);  
    addr.sin_addr.s_addr = inet_addr(IP);  
    if(connect(sockfd,(SA*)&addr,sizeof(addr))==-1){  
        perror("无法连接到服务器");  
        printf("客户端启动失败\n");  
        exit(-1);  
    }  
    printf("客户端启动成功\n");  
}  
  
  
  
void sig_close(int i) {  
    //关闭客户端的描述符  
    close(sockfd);  
    exit(0);  
}  

  
void send_pack(int type,char *send_name,char *recv_name,char *mes)  {  
    PACK pack_send_pack;  
    time_t timep;  
    pack_send_pack.type = type;  
    strcpy(pack_send_pack.data.send_name,send_name);  
    strcpy(pack_send_pack.data.recv_name,recv_name);  
    strcpy(pack_send_pack.data.mes,mes);   
    time(&timep);  
    pack_send_pack.data.time = timep;  
    if(send(sockfd,&pack_send_pack,sizeof(PACK),0) < 0){  
        my_err("send",__LINE__);  
    }  
}  
  
  
  
void send_pack_memcpy(int type,char *send_name,char *recv_name,char *mes) {  
    PACK pack_send_pack;  
    time_t timep;  
    pack_send_pack.type = type;  
    strcpy(pack_send_pack.data.send_name,send_name);  
    strcpy(pack_send_pack.data.recv_name,recv_name);  
    memcpy(pack_send_pack.data.mes,mes,MAX_CHAR*2);   
    time(&timep);  
    pack_send_pack.data.time = timep;  
    if(send(sockfd,&pack_send_pack,sizeof(PACK),0) < 0){  
        my_err("send",__LINE__);  
    }  
}  
  
int get_choice(char *input) {
    int len = strlen(input);
    int res = 0;

    for(int i = 0;i < len;i++) {
        if(input[i] >= '0' && input[i] <= '9') {
            int t = (int)(input[i] - 48);
            res = 10 * res + t;
        } else {
            return -1;
        }
    }
    return res;
}

int send_login(char username_t[],char password_t[]) {  
    PACK recv_login_t;  
    int login_judge_flag = 0;  
    //printf("password_t : %s \n",password_t);

    send_pack(LOGIN,username_t,"server",password_t);  

    //printf("login_judge_flag: %d\n",login_judge_flag);

    
    if(recv(sockfd,&recv_login_t,sizeof(PACK),0) < 0) { 
        my_err("recv",__LINE__);  
    }  
    

    login_judge_flag = recv_login_t.data.mes[0] - 48;  
    return login_judge_flag;  
}  
  
  
int login() {  
    int flag = 0;  
      
    int login_flag = 0;  
    char username_t [MAX_CHAR];  
    //char password_t [MAX_CHAR];  
    char *password_t;
    printf("please input the username:\n");  
    scanf("%s",username_t);  
    //printf("please input the password\n");  
    //scanf("%s",password_t);  
    password_t = getpass("please input the password\n");

    login_flag = send_login(username_t,password_t);  
    printf("login_flag : %d\n",login_flag);
    
    if(login_flag == 2){  
        printf("the username is not exit.\n");  
        return 0;  
    }     

    if(login_flag == 3){  
        printf("the user has loged in .\n");  
        return 0;  
    }    
    if(login_flag == 0) {      
        printf("the password is not crrect.\n");  
        return 0;  
    }  
    strcpy(now_info.username,username_t);  
    printf("load successfully!\n");  
    return 1;  
}  
  
  
int login_menu()  {  
    char choice_t[100];  
    int chioce;  
    do {  
        printf("\n\t\t*******************************\n");  
        printf("\t\t*        1.login in           * \n");  
        printf("\t\t*        2.register           * \n");  
        printf("\t\t*        0.exit               * \n");  
        printf("\t\t*******************************\n");  
        printf("\t\tchoice：");  
        scanf("%s",choice_t);  
        chioce = get_choice(choice_t);  
        switch(chioce) {    
            case 1:  
                if(login() == 1)  
                    return 1;  
                break;  
            case 2:  
                registe();  
                break;  
            default:  
                break;  
        }  
    }while(chioce != 0);  
    return 0;  
}  

  
int send_registe(char username_t[],char password_t[]) {  
    PACK recv_registe_t;  
    int send_registe_flag;  
      
    send_pack(REGISTER,username_t,"server",password_t);  
      
    if(recv(sockfd,&recv_registe_t,sizeof(PACK),0) < 0){  
        my_err("recv",__LINE__);  
    }  
    send_registe_flag = recv_registe_t.data.mes[0] - 48;  
    return send_registe_flag;  
}  
  

  
void registe() {  
    int flag = 0;  
    flag = REGISTER;  
    char username_t[MAX_CHAR];  
    //char password_t[MAX_CHAR];  
    char *password_t;
    
    printf("please input the username you want register:\n");  
    scanf("%s",username_t);  
    //printf("please input the password you want register:\n");  
    //scanf("%s",password_t);  
    password_t = getpass("please input the password you want register:\n");
    if(send_registe(username_t,password_t))  
        printf("registe successfully!\n");  
    else   
        printf("the name is used ,please input another one\n");  
}    
  
    
  
  
  
  
void change_statu(PACK pack_deal_statu_t) {  
    int count = 0;  
    now_info.friends_num = pack_deal_statu_t.data.mes[count++];  
  
    for(int i = 1; i <= now_info.friends_num ;i++) {  
        for(int j = 0;j < SIZE_PASS_NAME;j++) {  
            if(j == 0)     
                now_info.friends[i].statu = pack_deal_statu_t.data.mes[count+j] - 48;  
            else  
                now_info.friends[i].name[j-1] = pack_deal_statu_t.data.mes[count+j];  
        }  
        count += SIZE_PASS_NAME;  
    }  
  
    now_info.group_num=pack_deal_statu_t.data.mes[count++];  
    for(int i = 1 ;i <= now_info.group_num ;i++) {  
        for(int j = 0;j < SIZE_PASS_NAME;j++) {  
            now_info.group[i][j] = pack_deal_statu_t.data.mes[count+j];  
        }  
        count += SIZE_PASS_NAME;  
    }  
}  
  
  
  
void *deal_statu(void *arg) {  
    int i;  
    printf("function deal_statu:\n");  
    while(1) {  
        pthread_mutex_lock(&mutex_local_user);   
        for(i = 1;i <= show_friend_num;i++) {  
              
            change_statu(show_friend_pack[i]);  
        }  
        show_friend_num = 0;  
        pthread_mutex_unlock(&mutex_local_user);   
        usleep(1);   
    }  
}  


//从起始位置向服务端发送文件  
void send_file_send(int begin_location,char *file_path) {  
    int fd;  
    int length;  
    int file_size;  
    int sum = begin_location;  
    char mes[MAX_CHAR*2];  
    printf("\n\nsending the file.........\n");  
      
    //打开文件
    if((fd = open(file_path,O_RDONLY)) == -1) {  
        my_err("open",__LINE__);  
        return ;  
    }  
    file_size = lseek(fd, 0, SEEK_END);  
      
    printf("file_size=%d",file_size);  

    //文件内部指针移动  
    lseek(fd ,begin_location ,SEEK_SET);  
  
    bzero(mes, MAX_CHAR*2);   
  
    // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止   
    while((length = read(fd  ,mes + NUM_MAX_DIGIT ,MAX_CHAR*2 - NUM_MAX_DIGIT)) > 0) {  
        sum += length;  
        printf("length = %d\n", length);  
        int digit = 0;  
        while(length != 0) {     
            mes[digit++] = length % 10;  
            length /= 10;  
        }  
        mes[digit]  = -1;  
        printf("have sended : %d%%    %d  \n",(int)((double)sum/file_size*100),sum);  
        printf("%s\n", file_path);  
        send_pack_memcpy(FILE_SEND,now_info.username,file_path,mes);  
          
        if(sum == file_size)    
            break;  
        bzero(mes, MAX_CHAR*2);   
        usleep(100000);  
    }   
    // 关闭文件   
    close(fd);  
    printf("send finished!!\n");  
    printf("\n\t\t*********************************\n");  
    printf("\t\t*        1.show   friends       *\n");  
    printf("\t\t*        2.add    friends       *\n");  
    printf("\t\t*        3.delete friends       *\n");  
    printf("\t\t*        4.show   group         *\n");  
    printf("\t\t*        5.create group         *\n");  
    printf("\t\t*        6.join   group         *\n");  
    printf("\t\t*        7.quit   group         *\n");  
    printf("\t\t*        8.delete group         *\n");  
    printf("\t\t*        9.chat friend          *\n");  
    printf("\t\t*        10.chat group          *\n");  
    printf("\t\t*        11.send  file          *\n");  
    printf("\t\t*        12.message box         *\n");  
    printf("\t\t*        13.mes recording       *\n");
    printf("\t\t*        14.deal add friend     *\n");  
    printf("\t\t*        0.exit                 *\n");  
    printf("\t\t******************************* *\n");  
    printf("\t\tchoice：");  
  
}  


//允许发送时，开启线程发送文件 
void *pthread_send_file(void *mes_t) {  
    char *mes = (char *)mes_t;  
    int begin_location = 0;  
    char file_name[MAX_CHAR];  
    printf("\nfunction:pthread_send_file \n");  
    for(int i=0 ;i < NUM_MAX_DIGIT ;i++) {  
        if(mes[i] == -1)    
            break;  
        int t1 = 1;  
        for(int l = 0;l < i;l++)  
            t1 *= 10;  
        begin_location += (int)mes[i]*t1;  
  
    }  
    printf("pthread_send_file:%d\n",begin_location);  
    strcpy(file_name,mes+NUM_MAX_DIGIT);  
    printf("sdfa:%s\n", file_name);  
    send_file_send(begin_location,file_name);  
}  
  


//接收文件线程，  
//从存储接收包的地方检索到信息  
//并写入文件  
//当文件写入完成，关闭线程 
void *pthread_recv_file(void *par_t)  {  
    PTHREAD_PAR * pthread_par  = (PTHREAD_PAR * )par_t;  
    int file_size              = pthread_par->a ;  
    int begin_location_server  = pthread_par->b;  
    int sum                    = begin_location_server;   
    while(1) {
        pthread_mutex_lock(&mutex_recv_file);   
        int  fd;  
        char file_name[MAX_CHAR];  
        for(int i = 1;i <= recv_file_num ;i++) {  
            int  len = 0;  
            for(int j = 0 ;j < NUM_MAX_DIGIT ;j++) {  
                if(recv_pack_file[i].data.mes[j] == -1)    
                    break;  
                int t1 = 1;  
                for(int l = 0;l < j;l++)  
                    t1 *= 10;  
                len += (int)recv_pack_file[i].data.mes[j]*t1;  
            }  
              
            strcpy(file_name,recv_pack_file[i].data.send_name);  
            
            if((fd = open(file_name,O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0) {  
                my_err("open",__LINE__);  
                return NULL;  
            }  
  
            if(write(fd,recv_pack_file[i].data.mes + NUM_MAX_DIGIT,len) < 0)  
                my_err("write",__LINE__);  
            // 关闭文件   
            close(fd);  
            sum += len;  
            printf("have recved : %d%%    %d  \n",(int)((double)sum/file_size*100),sum);  
              
            recv_file_num = 0;  
            if(sum >= file_size) {  
                send_pack(FILE_FINI_RP,now_info.username,"server",file_name);  
                printf("send finished!!\n");  
                printf("send finished!!\n");  
                printf("\n\t\t*********************************\n");  
                printf("\t\t*        1.show   friends       *\n");  
                printf("\t\t*        2.add    friends       *\n");  
                printf("\t\t*        3.delete friends       *\n");  
                printf("\t\t*        4.show   group         *\n");  
                printf("\t\t*        5.create group         *\n");  
                printf("\t\t*        6.join   group         *\n");  
                printf("\t\t*        7.quit   group         *\n");  
                printf("\t\t*        8.delete group         *\n");  
                printf("\t\t*        9.chat friend          *\n");  
                printf("\t\t*        10.chat group          *\n");  
                printf("\t\t*        11.send  file          *\n");  
                printf("\t\t*        12.message box         *\n");  
                printf("\t\t*        13.mes recording       *\n");
                printf("\t\t*        14.deal add friend     *\n");  
                printf("\t\t*        0.exit                 *\n");  
                printf("\t\t******************************* *\n");  
                printf("\t\tchoice：");    
                return NULL;    
            }  
        }  
  
        pthread_mutex_unlock(&mutex_recv_file);  
        usleep(10);  
    }  
}  
  
int add_friend_flag;

void *client_recv_thread(void *arg) {  
    int i;  
    PACK pack_t;  
    pthread_t pid_send_file;  
    while(1) {  
        if(recv(sockfd,&pack_t,sizeof(PACK),0) < 0){  
            my_err("recv",__LINE__);  
        }  
          
        pthread_mutex_lock(&mutex_local_user);   
          
        for(int i = 1 ;i <= now_info.friends_num;i++) {  
            if(strcmp(now_info.friends[i].name,pack_t.data.send_name) == 0) {  
                now_info.friends[i].mes_num++;  
                break;  
            }  
        }  
  
        switch(pack_t.type) {  
            printf("client_recv_thread:%d\n", pack_t.type);  
            case SHOW_FRIEND:  
                show_friend_pack[++show_friend_num] = pack_t;  
                break;           
            case GROUP_CREATE:  
                group_create_flag = pack_t.data.mes[0];  
                printf("group_create_flag : %d\n",group_create_flag);
                break;  
            case GROUP_JOIN:  
                group_join_flag   = pack_t.data.mes[0];  
                break;  
            case GROUP_DEL:  
                group_delete_flag    = pack_t.data.mes[0];
                break;   
            case DEAL_FRIEND:
                //printf("收到了apply\n");
                friend_apply_pack[++friend_apply_pack_num]    = pack_t;
                break;
                
            /*
            case SHOW_GROUP_MEM:
                show_group_mem_pack[++show_group_mem_pack_num]        = pack_t;
                break;
            */
            case CHAT_ONE:  
                chat_pack[++recv_chat_num]                    = pack_t;  
                break;  
            case CHAT_MANY:  
                chat_pack[++recv_chat_num]                    = pack_t;  
                break;  
            /*    
            case SEND_FILE:  
                send_file_pack[++send_file_num]   = pack_t;  
                break;  
            */   
            case FILE_SEND_BEGIN_RP:  
                pthread_create(&pid_send_file,NULL,pthread_send_file,(void *)pack_t.data.mes);  
                break;  
            case FILE_SEND_STOP_RP:  
                recv_file_mes_pack[++recv_file_mes_num]             = pack_t;  
                break;  
            case FILE_RECV_BEGIN:  
                recv_file_mes_pack[++recv_file_mes_num]             = pack_t;  
                break;  
            case FILE_RECV:  
                pthread_mutex_lock(&mutex_recv_file);   
                recv_pack_file[++recv_file_num]                     = pack_t;                  
                printf("FILE_RECV:%d\n", recv_file_num);  
                pthread_mutex_unlock(&mutex_recv_file);  
                break;   
            case FILE_RECV_STOP_RP:  
                recv_file_mes_pack[++recv_file_mes_num]             = pack_t;  
                break;  
            case MES_RECORD:  
                print_mes_record(pack_t);  
                break;  
        }  
        pthread_mutex_unlock(&mutex_local_user);   
        usleep(1);   
    }  
}  
  
  
void init_client_pthread()  {  
    pthread_t pid_deal_statu,pid_recv,pid_recv_file;  
    pthread_create(&pid_deal_statu,NULL,deal_statu,NULL);  
    pthread_create(&pid_recv,NULL,client_recv_thread,NULL);  
}   
  
  
  
void get_status_mes()  {  //更新状态
    PACK show_friend_pack;  
    show_friend_pack.type = SHOW_FRIEND;  
    strcpy(show_friend_pack.data.send_name,now_info.username);  
    strcpy(show_friend_pack.data.recv_name,"server");  
    memset(show_friend_pack.data.mes,0,sizeof(show_friend_pack.data.mes));  
    if(send(sockfd,&show_friend_pack,sizeof(PACK),0) < 0){  
        my_err("send",__LINE__);  
    }  
}  
  
  
void show_friend()  {  
    pthread_mutex_lock(&mutex_local_user);  
    printf("friends num:%d\n", now_info.friends_num);   
    printf("\n\nthe list of friends:\n");  
    for(int i = 1 ;i <= now_info.friends_num ;i++)  {    
        if(now_info.friends[i].statu == ONLINE) {
            printf("ID:%d \033[;32m%s\033[0m[ONLINE] ", i,now_info.friends[i].name);  
            /*
            if(now_info.friends[i].mes_num)  
                printf("\033[;33m%d messages\033[0m\n", now_info.friends[i].mes_num);  
            else
                recv_file_mes_num += now_info.friends[i].mes_num;
            */
                printf("\n");  
        } else if(now_info.friends[i].statu == DOWNLINE) {
            printf("ID:%d \033[;31m%s\033[0m[DOWNLINE] ", i,now_info.friends[i].name);  
            /*
            if(now_info.friends[i].mes_num)  
                printf("\033[;33m%d messages\033[0m\n", now_info.friends[i].mes_num);  
            else   
                recv_file_mes_num += now_info.friends[i].mes_num;
                */
                printf("\n");  
        }  
    }  
    pthread_mutex_unlock(&mutex_local_user);    
}  
  
  
int judge_same_friend(char add_friend_t[]) {  
    int i;  
    for(i = 1;i <= now_info.friends_num;i++)  {  
        if(strcmp(now_info.friends[i].name,add_friend_t) == 0)  
            return i;  
    }  
    return 0;  
}  
  

/*
void apply_friend() {
    printf("please input the name of friend you want to add:\n");  
    scanf("%s",add_friend_t);  
    if(strcmp(add_friend_t,now_info.username) == 0)  {  
        printf("you can't add youself to be your friend!\n");  
        return;  
    }  
    if(judge_same_friend(add_friend_t)) {  
        printf("you already have same friends!\n");  
        return;  
    }  
    printf("now_info.username:%s\n", now_info.username);  
    send_pack(DEAL_FRIEND,now_info.username,"server",add_friend_t);  
}


*/  
/*
void add_friend() {  
    char add_friend_t[MAX_CHAR];  
      
    printf("please input the name of friend you want to add:\n");  
    scanf("%s",add_friend_t);  
    if(strcmp(add_friend_t,now_info.username) == 0)  {  
        printf("you can't add youself to be your friend!\n");  
        return;  
    }  
    if(judge_same_friend(add_friend_t)) {  
        printf("you already have same friends!\n");  
        return;  
    }  
    printf("now_info.username:%s\n", now_info.username);  
    
    send_pack(FRIEND_ADD,now_info.username,"server",add_friend_t);  
    get_status_mes();  
    printf("friend num: %d\n",now_info.friends_num);
}  
*/
void apply_friend() {
    char add_friend_t[MAX_CHAR];
    printf("please input the name of friend you want to add:\n");  
    scanf("%s",add_friend_t);  
    if(strcmp(add_friend_t,now_info.username) == 0)  {  
        printf("you can't add youself to be your friend!\n");  
        return;  
    }  
    if(judge_same_friend(add_friend_t)) {  
        printf("you already have same friends!\n");  
        return;  
    }  
    printf("now_info.username:%s\n", now_info.username);  
    send_pack(DEAL_FRIEND,now_info.username,"server",add_friend_t); 
    printf("发包成功了\n"); 
}

void add_friend() {
    char choice[MAX_CHAR];
    printf("\n\n");
    //printf("%d   \n",friend_apply_pack_num);
    if(friend_apply_pack_num == 0) {
        return;
    }
    //printf("%s\n",friend_apply_pack[0].data.mes);
    for(int i = 1;i <= friend_apply_pack_num;i++) {
        printf("\t\t  %s want to apply you as friend\n",friend_apply_pack[i].data.mes);
    
        printf("\t\t  input Y or y if you add this friend\n");
        scanf("%s",choice);
        char ch = choice[0];
        if(ch == 'Y' || ch == 'y') {
            send_pack(FRIEND_ADD,now_info.username,"server",friend_apply_pack[i].data.mes);
        }
    } 
}
  
void delete_friend() {  
    char delete_friend_t[MAX_CHAR];  
    printf("please input the name of friend you want to delete:\n");  
    scanf("%s",delete_friend_t);  
  
    if(!judge_same_friend(delete_friend_t)) {  
        printf("you don't have this friends on list!\n");  
        return ;  
    }  
    printf("now_info.username:%s\n", now_info.username);  
      
    send_pack(FRIEND_DEL,now_info.username,"server",delete_friend_t);  
      
    get_status_mes();  
}  
  
void show_split(char *name ,char *time ,char *mes) {  
    int number = 6;  
    if(print_mes_num == number) {  //数据存满则加在第一个
        for(int i = 1;i <= 5;i++)  
            print_mes_info[i] = print_mes_info[i+1];  
        strcpy(print_mes_info[number].name,name);  
        strcpy(print_mes_info[number].time,time);  
        strcpy(print_mes_info[number].mes,mes);  
    } else{                        //没满直接添加
         strcpy(print_mes_info[++print_mes_num].name,name);  
         strcpy(print_mes_info[print_mes_num].time,time);  
         strcpy(print_mes_info[print_mes_num].mes,mes);  
    }  
  
    //记录光标的信息
    printf("\33[s");  
    fflush(stdout);  
  
    //光标移动到顶端
    printf("\33[25A\n");    
      
    //清空显示区的信息
    for(int i = 1;i <= 20;i++)  
        printf("\33[K\n");  
  
    //回到行顶
    printf("\33[21A\n");  

    //打印信息
    for(int i = 1;i <= print_mes_num;i++) {  
        printf("\033[40;42m%s\033[0m\t%s",print_mes_info[i].name,print_mes_info[i].time);  
        printf("%s\n", print_mes_info[i].mes);  
    }  
    //不足则用空行代替
    for(int i = 1;i <= 6 - print_mes_num ;i++) {  
        printf("\n");  
        printf("\n\n");  
    }  

    //回到原来的位置 
    printf("\33[u");  
    fflush(stdout);  
}  
  
int judge_same_group(char *group_name) {
    int i;
    for(i = 1 ;i <= now_info.group_num;i++) {
        if(strcmp(now_info.group[i],group_name) == 0) {
            return 1;
        }
    }

    return 0;
}  
  
  
void show_group() {  
    pthread_mutex_lock(&mutex_local_user);   
    printf("\n\nthe list of group:\n");  
    for(int i=1 ;i<=now_info.group_num ;i++) {  
        printf("ID:%d %s\n", i,now_info.group[i]);  
    }  
    pthread_mutex_unlock(&mutex_local_user);    
}  


void send_mes(char mes_recv_name[],int type) {  
    PACK pack_send_mes;  
    char mes[MAX_CHAR];  
    time_t timep;  
    getchar();  
    printf("**********************************************************\n");  
    while(1)  {     
        time(&timep);  
        memset(mes,0,sizeof(mes));  
          fgets(mes,MAX_CHAR,stdin);  
        while(mes[0] == 10)  {  
            printf("\33[1A");  
            fflush(stdout);  
            fgets(mes,MAX_CHAR,stdin);  
        }  
        if(strcmp(mes,"exit\n") == 0)  
            break;  
        printf("\33[1A");  
        printf("\33[K");  
        fflush(stdout);  
        show_split(now_info.username ,ctime(&timep) ,mes);  
          
         
  
        send_pack(type,now_info.username,mes_recv_name,mes);  
    }  
    print_mes_flag = EXIT;  
}  
  
  
void print_mes(int id) {  
    char group_print_name[MAX_CHAR];  
    memset(group_print_name,0,sizeof(group_print_name));  
    if(chat_pack[id].type == CHAT_ONE) {  
        show_split(chat_pack[id].data.send_name,ctime(&chat_pack[id].data.time),chat_pack[id].data.mes);  
    }  
    else {  
        for(int i = 0;i < SIZE_PASS_NAME;i++) {  
            group_print_name[i] = chat_pack[id].data.mes[i];  
        }  
        show_split(group_print_name,ctime(&chat_pack[id].data.time),chat_pack[id].data.mes + SIZE_PASS_NAME);  
   }  
}  
  
    
void *show_mes(void *username) {  
    int id;  
    char *user_name = (char *)username;  
    while(1) {  
        if (print_mes_flag == EXIT)  
            break;  
        pthread_mutex_lock(&mutex_local_user);   
        id = 0;  
        for(int i = 1 ;i <= recv_chat_num; i++) {  
            if(strcmp(chat_pack[i].data.send_name,user_name) == 0) {  
                id = i;  
                print_mes(id);  
                recv_chat_num--;  
                for(int j = id; j <= recv_chat_num && recv_chat_num ;j++) {  
                    chat_pack[j]  =  chat_pack[j+1];  
                }  
                break;  
            }  
        }  
          
        pthread_mutex_unlock(&mutex_local_user);   
        usleep(1);      
    }  
}  
  
  
  
void chat_with_one() {  
    pthread_t pid;  
    int id;  
    char mes_recv_name[MAX_CHAR];  
    show_friend();  
  
    printf("please input the name you want to chat\n");  
    scanf("%s",mes_recv_name);  
    if (!(id=judge_same_friend(mes_recv_name)))  {  
        printf("sorry,you don't have the friend named !%s\n",mes_recv_name);  
        return ;  
    }  
    printf("\33[2J \33[30A ***********************************************");  //清屏到行顶
    printf("\33[23B\n");  
    print_mes_flag = 1;  
    now_info.friends[id].mes_num = 0;  
    pthread_create(&pid,NULL,show_mes,(void *)mes_recv_name);  
    send_mes(mes_recv_name,CHAT_ONE);  
}  
  
  
void group_create() {  
    char group_name[MAX_CHAR];  
    printf("please input the group name you want to create:\n");  
    scanf("%s",group_name);  
    send_pack(GROUP_CREATE,now_info.username,"server",group_name);  
    while(!group_create_flag);  
    printf("group_create_flag=%d\n", group_create_flag);  
    if(group_create_flag == 2)   
        printf("create group successfully!\n");  
    else if(group_create_flag == 1)  
        printf("this group has been created!\n");  
    group_create_flag = 0;  
}  
  
  
  
void group_join() {  
    char group_name[MAX_CHAR];  
    printf("please input the group name you want to join:\n");  
    scanf("%s",group_name);  
      
    for(int i = 1;i <= now_info.group_num ;i++)  {  
        if(strcmp(now_info.group[i],group_name) == 0) {  
            printf("you have join this group!\n");  
            return ;  
        }  
    }  
  
    send_pack(GROUP_JOIN,now_info.username,"server",group_name);  
    while(!group_join_flag);  
    printf("group_join_flag=%d\n", group_join_flag);  
    if(group_join_flag == 2)   
        printf("join group successfully!\n");  
    else if(group_join_flag == 1)  
        printf("there is no group named %s\n",group_name);  
    group_join_flag = 0;  
}  
  
  
void group_quit() {  
    char group_name[MAX_CHAR];  
    printf("please input the group name you want to qiut:\n");  
    scanf("%s",group_name);  
      
    for(int i = 1;i <= now_info.group_num ;i++) {  
        if(strcmp(now_info.group[i],group_name) == 0) {  
            send_pack(GROUP_QUIT,now_info.username,"server",group_name);  
            printf("quit group %s successfully!\n",group_name);  
            return ;  
        }  
    }  
      
    printf("you don't join this group!\n");  
}  
  
  
void group_del() {  
    char group_name[MAX_CHAR];  
    printf("please input the group name you want to delete:\n");  
    scanf("%s",group_name);  
    for(int i = 1;i <= now_info.group_num ;i++)  {  
        if(strcmp(now_info.group[i],group_name) == 0)  {  
            send_pack(GROUP_DEL,now_info.username,"server",group_name);  
            while(!group_delete_flag);  
            printf("group_delete_flag=%d\n", group_delete_flag);  
            if(group_delete_flag == 2)   
                printf("delete group successfully!\n");  
            else if(group_delete_flag == 1)  
                printf("you isn't the owner of group %s\n",group_name);  
            return ;  
        }  
    }  
      
    printf("you don't join this group!\n");  
}  
  
  
  
  
void chat_with_group() {  
    pthread_t pid;  
    char mes_recv_group_name[MAX_CHAR];  
    show_group();  
    printf("please input the group you want to chat\n");  
    scanf("%s",mes_recv_group_name);  
    if (!judge_same_group(mes_recv_group_name))  {  
        printf("sorry,you don't have the group named !%s\n",mes_recv_group_name);  
        return ;  
    }  
  
    print_mes_flag = 1;  
    printf("\33[2J \33[30A ***********************************************");  
    printf("\33[23B\n");  
    pthread_create(&pid,NULL,show_mes,(void *)mes_recv_group_name);  
    send_mes(mes_recv_group_name,CHAT_MANY);  
    
}  
  
  
  
int get_file_size(char *file_name) {  
    int fd;  
    int len;  
    if((fd = open(file_name,O_RDONLY)) == -1)  
    {  
        my_err("open",__LINE__);  
        return 0;  
    }  
    len = lseek(fd, 0, SEEK_END);  
    close(fd);  
    return len;  
}  
  

//向服务端请求发送文件，并将文件的大小 
//发送给服务端  
void send_file() {  
    char  recv_name[MAX_CHAR];  
    char  file_path[MAX_CHAR];  
    int   file_size_t;  
    char  mes_t[MAX_CHAR];  
  
    printf("please input the friend name:\n");  
    scanf("%s",recv_name);  
  
    int id = judge_same_friend(recv_name);  
    if(id == 0) {  
        printf("you don't hava this friend!\n");  
        return;  
    }  
    printf("please input the path of file you want to send :\n");  
    scanf("%s",file_path);  
      
    file_size_t = get_file_size(file_path);  
    printf("file_size :%d\n", file_size_t);  
    
    //得到文件大小
    if(file_size_t == 0)  {  
        printf("please input creact file path\n");  
        return ;  
    }  
  
    //字符串处理
    int digit = 0;  
    while(file_size_t != 0)  {     
        mes_t[digit++] = file_size_t % 10;  
        file_size_t /= 10;  
    }  
    mes_t[digit]  = -1;  
    //文件大小 
  
    for(int i = 0 ;i < SIZE_PASS_NAME ;i++) {  
        mes_t[NUM_MAX_DIGIT+i] = file_path[i];  
    }  
    //文件名
    printf("file_path:%s\n",mes_t+ NUM_MAX_DIGIT);  
    //发送请求
    send_pack_memcpy(FILE_SEND_BEGIN,now_info.username,recv_name,mes_t);  
}  
  
//删除文件消息
void file_infor_delete(int id) {  
    pthread_mutex_lock(&mutex_local_user);   
    for(int j = id ;j <= recv_file_mes_num ;j++)  {  
        recv_file_mes_pack[j]  = recv_file_mes_pack[j+1];  
    }  
    recv_file_mes_num--;  
    pthread_mutex_unlock(&mutex_local_user);   
}  
  
  
//上传失败是否重新上传
void mes_sendfile_fail(int id) {  
    char chioce[10];  
    int begin_location = 0;  
    for(int i = 0 ;i < NUM_MAX_DIGIT ;i++) {  
        if( recv_file_mes_pack[id].data.mes[i] == -1)    
            break;  
        printf("%d\n\n",recv_file_mes_pack[id].data.mes[i]);  
        int t1 = 1;  
        for(int l=0;l<i;l++)  
            t1*=10;  
        begin_location += (int)recv_file_mes_pack[id].data.mes[i] * t1;  
    }  
  
    int file_size_t = get_file_size(recv_file_mes_pack[id].data.send_name);  
    printf("the file %s send failed ,have sended %d%%,do you want send again?\n", recv_file_mes_pack[id].data.send_name,(int)((double)begin_location/file_size_t*100));  
    printf("y/n :");  
    scanf("%s",chioce);  
      
  
    if(chioce[0] != 'Y' && chioce[0] != 'y') {  
        file_infor_delete(id);  
        return ;  
    }  
      
      
    printf("&&&&&&&\n begin_location :%d\n",begin_location);  
  
  
    send_file_send(begin_location,recv_file_mes_pack[id].data.send_name);  
    file_infor_delete(id);  
}  
  
//是否接收文件 
void mes_recv_requir(int id)  {  
    pthread_t  pid_recv_file;  
    char choice[10];  
    int len ;  
    int fd;  
    char mes_t[MAX_CHAR];  
    int file_size = 0;  
    char file_name[SIZE_PASS_NAME];  
      
    PTHREAD_PAR * par_t = (PTHREAD_PAR *)malloc(sizeof(PTHREAD_PAR));  
  
    for(int i = 0 ;i < NUM_MAX_DIGIT ;i++)  {  
        if(recv_file_mes_pack[id].data.mes[i] == -1)    
            break;  
        int t1 = 1;  
        for(int l = 0;l < i;l++)  
            t1 *= 10;  
        file_size += (int)recv_file_mes_pack[id].data.mes[i]*t1;  
    }       
    strcpy(file_name,recv_file_mes_pack[id].data.mes+NUM_MAX_DIGIT);  
  
      
    printf("  %s send file %s size(%db)to you \n", recv_file_mes_pack[id].data.send_name,file_name,file_size);  
    printf(" do you want receive it? \n");  
    printf("(y/n) :");  
    scanf("%s", choice);  
    if(choice[0] != 'Y' && choice[0] != 'y')  {  
        file_infor_delete(id);            
        return ;  
    }  
      
    if((fd = open(file_name,O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR)) < 0) {  
        my_err("open",__LINE__);  
        return ;  
    }  
    len = lseek(fd, 0, SEEK_END);  
    close(fd);  
  
    par_t->a = file_size;  
    par_t->b = len;  
      
    int digit = 0;  
    while(len != 0) {     
        mes_t[digit++] = len%10;  
        len /= 10;  
    }  
    mes_t[digit]  = -1;  
      
    send_pack_memcpy(FILE_SEND_BEGIN_RP ,now_info.username ,file_name ,mes_t);  
      
    pthread_create(&pid_recv_file,NULL,pthread_recv_file,(void *)par_t);  
  
    file_infor_delete(id);  
}  
  
  
//文件中断是否续传 
void mes_recvfile_fail(int id) {  
    pthread_t  pid_recv_file;  
    char chioce[10];  
    int begin_location_server;  
    int file_size;  
    char file_name[SIZE_PASS_NAME];  
    char mes_t[MAX_CHAR];  
    PTHREAD_PAR * par_t = (PTHREAD_PAR *)malloc(sizeof(PTHREAD_PAR));  
  
    for(int i = 0 ;i < NUM_MAX_DIGIT ;i++) {  
        if(recv_file_mes_pack[id].data.mes[i] == -1)    
            break;  
        int t1 = 1;  
        for(int l = 0;l < i;l++)  
            t1*=10;  
        file_size += (int)recv_file_mes_pack[id].data.mes[i]*t1;   
    }     
  
    strcpy(file_name,recv_file_mes_pack[id].data.mes+NUM_MAX_DIGIT);  
      
    begin_location_server= get_file_size(file_name);  
      
  
    par_t->a = file_size;  
    par_t->b = begin_location_server;  
    printf("the file %s recv failed ,have recved %d%%,do you want recv continue?\n", file_name,(int)((double)begin_location_server/file_size*100));  
    printf("y/n :");  
    scanf("%s",chioce);  
      
      
  
    if(chioce[0] != 'Y' && chioce[0] != 'y')  {    
        file_infor_delete(id);   
          
        return ;  
    }  
      
      
    printf("&&&&&&&\nbegin_location :%d\n",begin_location_server);  
  
    int len = begin_location_server;  
    int digit = 0;  
    while(len != 0)  {     
        mes_t[digit++] = len%10;  
        len /= 10;  
    }  
    mes_t[digit]  = -1;  
  
    send_pack_memcpy(FILE_SEND_BEGIN_RP ,now_info.username ,file_name ,mes_t);  
      
    pthread_create(&pid_recv_file,NULL,pthread_recv_file,(void *)par_t);  
  
    file_infor_delete(id);    
}  


  
void deal_file_mes(int id)  {  
    if(recv_file_mes_pack[id].type == FILE_SEND_STOP_RP)  {  
        mes_sendfile_fail(id);  
    }  
    else if(recv_file_mes_pack[id].type == FILE_RECV_BEGIN)  {  
        mes_recv_requir(id);  
    }else if(recv_file_mes_pack[id].type == FILE_RECV_STOP_RP)  {  
        mes_recvfile_fail(id);  
    }  
}  
  
  
int file_mes_box()  {  
    char choice_t[100];  
    int chioce;  
    do {  
        get_status_mes();  
        printf("pack num_chat:%d\n", recv_chat_num);  
        printf("\n\t\t************message mes box *************\n");  
        for(int i = 1; i <= recv_file_mes_num;i++) {  
            if(recv_file_mes_pack[i].type == FILE_SEND_STOP_RP)  
                printf("\t\t        send file %s filed       \n",recv_file_mes_pack[i].data.send_name);  
            if(recv_file_mes_pack[i].type == FILE_RECV_BEGIN)  
                printf("\t\t        %s send file %s to you       \n", recv_file_mes_pack[i].data.send_name,recv_file_mes_pack[i].data.mes+SIZE_PASS_NAME);  
            if(recv_file_mes_pack[i].type == FILE_RECV_STOP_RP)  
                printf("\t\t         recv file %s filed      \n", recv_file_mes_pack[i].data.mes+NUM_MAX_DIGIT);  
           
        }  
        for(int i = 1 ;i <= now_info.friends_num ;i++)  {    
            if(now_info.friends[i].statu == ONLINE) {
                if(now_info.friends[i].mes_num)  {
                    printf("\t\t     \033[;32m%s\033[0m[ONLINE] ", now_info.friends[i].name);
                    printf("\033[;33m%d messages\033[0m\n", now_info.friends[i].mes_num);  
                }  
            } else if(now_info.friends[i].statu == DOWNLINE) {
                if(now_info.friends[i].mes_num)  {
                    printf("\t\t    \033[;31m%s\033[0m[DOWNLINE] ", now_info.friends[i].name);  
                    printf("\033[;33m%d messages\033[0m\n", now_info.friends[i].mes_num);    
                }
            }  
        }  
        printf("\t\t         0.exit                  \n");  
        printf("\t\t******************************* *\n");  
        printf("\t\tchoice：");  
        scanf("%s",choice_t);  
        chioce = get_choice(choice_t);  
        if(chioce != -1)     
            deal_file_mes(chioce);  
    } while(chioce != 0);  
    return 0;  
}  

  
void print_mes_record(PACK pack_t)  {  
    if(strcmp(pack_t.data.send_name ,"server") == 0)  {  
        printf("\033[40;42m%s\033[0m\t%s",pack_t.data.mes,pack_t.data.mes + 1 * SIZE_PASS_NAME);  
        printf("%s\n", pack_t.data.mes + 2 * SIZE_PASS_NAME);  
    }  
    else   
        print_record_flag = 1;  
}  
  
int mes_record() {  
    char username[MAX_CHAR];  
    printf("please input the username that you want see:\n");  
    scanf("%s",username);  
    printf("the recording :\n");  
    send_pack(MES_RECORD,now_info.username,"server",username);  //通过收包打印
    printf("\33[2J\33[20A********message recordes**************\n"); //清屏 
    while(!print_record_flag);  
    printf("printf recordes finished!!\n");  
    print_record_flag = 0;  
    return 0;  
}  
  
  
int main_menu() {  
    char choice_t[100];  
    int chioce;  
    do {  
        get_status_mes();  
       
        printf("\n\t\t*******************************\n");  
        printf("\t\t*        1.show   friends      *\n");  
        printf("\t\t*        2.add    friends      *\n");  
        printf("\t\t*        3.delete friends      *\n");  
        printf("\t\t*        4.show   group        *\n");  
        printf("\t\t*        5.create group        *\n");  
        printf("\t\t*        6.join   group        *\n");  
        printf("\t\t*        7.quit   group        *\n");  
        printf("\t\t*        8.delete group        *\n");  
        printf("\t\t*        9.chat with one       *\n");  
        printf("\t\t*        10.chat with many     *\n");  
        printf("\t\t*        11.send  file         *\n");  
        printf("\t\t*        12.message box        *\n");  
        printf("\t\t*        13.mes recording      *\n");
        printf("\t\t*        14.deal add friend    *\n");  
        printf("\t\t*        0.exit                *\n");  
        printf("\t\t********************************\n");  
        printf("\t\tchoice：");  
        scanf("%s",choice_t);  
        chioce = get_choice(choice_t);  
        switch(chioce) {    
            case 1:  
                show_friend();  
                break;  
            case 2:  
                //add_friend();  
                apply_friend();
                break;  
            case 3:  
                delete_friend();  
                break;  
            case 4:  
                show_group();  
                break;  
            case 5:  
                group_create();  
                break;  
            case 6:  
                group_join();  
                break;  
            case 7:  
                group_quit();  
                break;  
            case 8:  
                group_del();  
                break;  
            case 9:  
                chat_with_one();  
                break;  
            case 10:  
                chat_with_group();  
                break;  
            case 11:  
                send_file();  
                break;  
            case 12:  
                file_mes_box();  
                break;  
            case 13:  
                mes_record();  
                break;
            case 14:
                printf("好着呢\n");
                add_friend();
            default:  
                break;  
        }  
    } while(chioce != 0);  
    return 0;  
}  

  
int main(int argc, char const *argv[])  {  
    int flag =0;  
      
    signal(SIGINT,sig_close);//关闭CTRL+C  
    init();//启动并连接服务器  
      
    if(login_menu() == 0)    
        return 0;     
    init_client_pthread();  
    main_menu();  
  
    return 0;  
}  
