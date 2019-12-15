#include    <stdio.h>
#include     <termios.h>
#include    <stdlib.h>
#include    <netdb.h>
#include    <errno.h>
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h>
#include     <sys/socket.h>
#include     <ctype.h>
#include     <unistd.h>
#include     <string.h>
#include     <sys/ioctl.h>
#include    <arpa/inet.h>
#include <math.h>
#define ECHOMAX 255

char    *ftphostName;
char    return_bufArr[520];
double  speed=1.0;
int     bp=0;
char    *type = "A";
int getpasswd(char *pwd, int size);
int getusername(char *user, int size);
int mygetch();
int cliopen( char *ftphostName);
int quit_toFtp( int sockCs);
int ml_type( int sockCs, char *modeType );
int ml_cwd( int sockCs, char *pathInFtp );
int ml_cdup( int sockCs );
int ml_mkd( int sockCs, char *pathInFtp );
int ml_list( int sockCs);
int ml_get( int sockCs, char *s,double speed);
int ml_put( int sockCs, char *s,double speed);
int ml_renamefile( int sockCs, char *s, char *d );
int ml_deletefile( int sockCs, char *s );



int mygetch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int getpasswd(char *pwd, int size)
{
    int c, n = 0;
    do
    {
        c = mygetch();
        if (c != '\n' && c != '\r' && c != 127)
        {
            pwd[n] = c;
            //printf(" ");
            n++;
        }
        else if ((c != '\n' | c != '\r') && c == 127)//判断是否是回车或则退格
        {
            if (n > 0)
            {
                n--;
                printf("\b \b");//输出退格
            }
        }
    }while (c != '\n' && c != '\r' && n < (size - 1));
    pwd[n] = '\0';//消除一个多余的回车
    return n;
}
int getusername(char *user, int size)
{
    int c, n = 0;
    do
    {
        c = mygetch();
        if (c != '\n' && c != '\r' && c != 127)
        {
            user[n] = c;
            printf("%c",c);
            n++;
        }
        else if ((c != '\n' | c != '\r') && c == 127)//判断是否是回车或则退格
        {
            if (n > 0)
            {
                n--;
                printf("\b \b");//输出退格
            }
        }
    }while (c != '\n' && c != '\r' && n < (size - 1));
    user[n] = '\0';//消除一个多余的回车
    return n;
}


// 连接指定host
int
cliopen( char *ftphostName)
{
    int returnNum;
    int     sockCs;
    printf("Connect to %s .\n",ftphostName);
    sockCs = connect_server( ftphostName, 21 );
    if ( sockCs == -1 ) return -1;
    
    char userArr[126] ;
    char pwdArr[126] ;
    // 输入用户名密码
    printf("Your name:");
    getusername(userArr, 126);
    printf("\n");
    printf("Your password:");
    getpasswd(pwdArr, 126);
    printf("\n");
    returnNum=login_server( sockCs, userArr, pwdArr );
    while( returnNum==530) {
        printf("Login error. Please check your username and password.\n");
        printf("Your name:");
        getusername(userArr, 126);
        printf("\n");
        printf("Your password:");
        getpasswd(pwdArr, 126);
        printf("\n");
        returnNum=login_server( sockCs, userArr, pwdArr );
        if (returnNum==421){
            printf("Maximum login limit has reached.\n");
            return -1;
        }
    }
    return sockCs;
}


/* 创建TCP连接 */
int socket_connect(char *ftphostName,int port)
{
    int skfd;
    int fd;
    struct sockaddr_in sockAddr;
    int bufferNum;
    struct in_addr input_addr;
    struct hostent *hostinfo;
    ///create tcp socket
    if((skfd=socket(AF_INET,SOCK_STREAM,0)) < 0) {
        printf("socket failed\n");
        exit(1);
    } else {
        printf("socket success!\n");
    }
    
    //create address structure
    memset(&sockAddr, 0, sizeof(struct sockaddr_in));
    sockAddr.sin_family = AF_INET;
    //sockAddr.sin_addr.s_addr = inet_addr(ftphostName);
    sockAddr.sin_port = htons((unsigned short)port);

    
    if ((inet_aton(ftphostName,&input_addr))==0)
    {
        
        //lookup host infomation by domain name.
         hostinfo = gethostbyname(ftphostName);
         memcpy(&sockAddr.sin_addr.s_addr, hostinfo->h_addr, hostinfo->h_length);
    }
    else
    {
         sockAddr.sin_addr.s_addr = inet_addr(ftphostName);
    }
    
    //connect
    if(connect(skfd, (struct sockaddr *)(&sockAddr), sizeof(struct sockaddr)) < 0) {
        printf("connect error!\n");
        return -1;
    } else {
        printf("connnect success!\n");
    }
    return skfd;
}

// 连接服务端
int connect_server( char *ftphostName, int port )
{
    int       sockCtrlNum;
    char      bufArr[512];
    int       returnNum;
    ssize_t   len;
    
    sockCtrlNum = socket_connect(ftphostName, port);
    
    len = recv( sockCtrlNum, bufArr, 512, 0 );
    bufArr[len] = 0;
    printf("%s",bufArr);
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum != 220 ) {
        printf("connect error.\n");
        close( sockCtrlNum );
        return -1;
    }
    
    return sockCtrlNum;
}

// 发送命令
int sendCommandToftp_re( int sock, char *cmd, void *re_bufArr, ssize_t *len)
{
    ssize_t     r_len;
    
    if ( send( sock, cmd, strlen(cmd), 0 ) == -1 )
        return -1;
    
    r_len = recv( sock, return_bufArr, 512, 0 );
    if ( r_len < 1 ) return -1;
    return_bufArr[r_len] = 0;
    
    if (len != NULL) *len = r_len;
    if (re_bufArr != NULL) sprintf(re_bufArr, "%s", return_bufArr);
    
    return 0;
}

// 发送命令
int sendCommandToftp( int sock, char *cmd )
{
    bzero(return_bufArr, sizeof(return_bufArr));
    char     bufArr[512];
    int      returnNum;
    ssize_t  len;
    
    returnNum = sendCommandToftp_re(sock, cmd, bufArr, &len);
    if (returnNum == 0)
    {
        sscanf( bufArr, "%d", &returnNum );//格式化转化为int
    }
    
    return returnNum;
}

// 登录
int login_server( int sock, char *user, char *pwd )
{
    char    bufArr[128];
    int     returnNum;
    
    sprintf( bufArr, "USER %s\r\n", user );
    returnNum = sendCommandToftp( sock, bufArr );
    if ( returnNum == 230) {
        return 0;
    }
    else if ( returnNum == 331 ) {
        sprintf( bufArr, "PASS %s\r\n", pwd );
        returnNum=sendCommandToftp( sock, bufArr );
        printf("%d\n",returnNum);
        if((returnNum!=230)&&(returnNum!=202)){
            return returnNum;
        }
        return 0;
    }
    else
        return returnNum;
}

// active mode
int ml_list_active( int sockCtrlNum )
{
    int sockLsnNum;
    struct sockaddr_in sockAddr, cltAddr;
    socklen_t addrLen;
    char    cmd[128];
    int     port;
    char    bufArr[512];
    int     send_re;
    struct sockaddr_in sin;
    int     d_sock;
    int     returnNum;
    ssize_t     len,bufArr_len,total_len;
    void ** data ;
    unsigned long long *data_len ;
    
    //create tcp socket, 监听的socket
    if((sockLsnNum = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket created failed\n");
        return -1;
    } else {
        printf("socket created success!\n");
    }
    
    
    //create address structure
    memset(&sockAddr, 0, sizeof(struct sockaddr_in));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(13272);
    
    //bind
    if(bind(sockLsnNum, (struct sockaddr *)(&sockAddr), sizeof(struct sockaddr)) < 0) {
        printf("bind failed\n");
        return -1;
    } else {
        printf("bind success!\n");
    }
    
    //listen
    if(listen(sockLsnNum, 2) < 0) {
        printf("listen failed\n");
        return -1;
    } else {
        printf("listen success!\n");
    }
    sprintf( cmd, "PORT 10,128,251,159,51,216\r\n");
    if ( sendCommandToftp( sockCtrlNum, cmd ) != 200 ) {
        printf("Active mode failure.\n");
        return -1;
    }
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "LIST\r\n");
    send_re = sendCommandToftp(sockCtrlNum, bufArr );
    printf("%d\n",send_re);
    if (send_re >= 300 || send_re == 0){
        printf("%s\n",return_bufArr);
        return send_re;
    }
    addrLen = sizeof(struct sockaddr_in);
    if((d_sock = accept(sockLsnNum, (struct sockaddr *)(&cltAddr), &addrLen)) < 0) {//接收数据的socket
        printf("accept failed\n");
        return -1;
    } else {
        printf("accept success!\n");
    }

    len=total_len = 0;
    bufArr_len = 512;
    void *re_bufArr = malloc(bufArr_len);//分配了给re_bufArr512的空间
    while ( (len = recv( d_sock, bufArr, 512, 0 )) > 0 )
    {
        if (total_len+len > bufArr_len)
        {
            bufArr_len *= 2;
            void *re_bufArr_n = malloc(bufArr_len);
            memcpy(re_bufArr_n, re_bufArr, total_len);
            free(re_bufArr);
            re_bufArr = re_bufArr_n;
        }
        if (bufArr!=NULL)
            printf("%s\n",bufArr);
        memcpy(re_bufArr+total_len, bufArr, len);
        total_len += len;
    }
    close( d_sock );
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCtrlNum, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum != 226 )
    {
        free(re_bufArr);
        return returnNum;
    }
    close(sockLsnNum);
    return 0;
}

// 被动模式
int pasvMode( int sockCs )
{
    int     r_sock;
    int     send_re;
    ssize_t len;
    int     addr[6];
    char    bufArr[512];
    char    re_bufArr[512];
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "PASV\r\n");
    send_re = sendCommandToftp_re( sockCs, bufArr, re_bufArr, &len);
    if (send_re == 0) {
        // 服务端开放端口
        sscanf(re_bufArr, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
        printf("%s\n",re_bufArr);
    }
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    r_sock = socket_connect(bufArr,addr[4]*256+addr[5]);
    return r_sock;
}

int
ml_type( int sockCs, char *modeType )
{
    char    bufArr[128];
    int     re;
    sprintf( bufArr, "TYPE %s\r\n", modeType );
    re = sendCommandToftp( sockCs, bufArr );
    if (re == 200 ){
        bzero(bufArr, sizeof(bufArr));
        printf("%s\n",return_bufArr);
        return 1;
    }
    else
        printf("%s\n",return_bufArr);
        return -1;
}

// pwd命令
int
ftp_pwd( int sockCs)
{
    int  d_sock;
    char bufArr[512];
    int re;
    int returnNum;
    ssize_t  len,bufArr_len,total_len;
    void ** data;
    unsigned long long *data_len;
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "PWD\r\n");
    re = sendCommandToftp(sockCs, bufArr);
    if (re >= 300 || re == 0){
        printf("%s\n",return_bufArr);
        return re;
    }
    bzero(bufArr, sizeof(bufArr));
    sscanf(return_bufArr, "%*s%s", bufArr);
    printf("%s\n",bufArr);
    return 0;
}

// cwd命令
int
ml_cwd( int sockCs, char *pathInFtp )
{
    char    bufArr[128];
    int     re;
    sprintf( bufArr, "CWD %s\r\n", pathInFtp );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 250 ){
        printf("The path doesn't exist.\n");
        return re;
    }
    else
        return 0;
}

// cdup命令
int
ml_cdup( int sockCs )
{
    int     re;
    re = sendCommandToftp( sockCs, "CDUP\r\n" );
    if ( re != 250 )
    {
        printf("It is already the ultimate directory, you can't return back.\n");
        return re;
    }
    else
        return 0;
}

// mkdir 命令
int
ml_mkd( int sockCs, char *pathInFtp )
{
    char    bufArr[512];
    int     re;
    sprintf( bufArr, "MKD %s\r\n", pathInFtp );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re == 550 ){
        printf("Directory already existed or you have no permission to make a directory.\n");
        return re;
    }
    else
        return 0;
}


// ll命令
int
ml_list_passive( int sockCs)
{
    int     d_sock;
    char    bufArr[512];
    int     send_re;
    int     returnNum;
    ssize_t   len,bufArr_len,total_len;
    void ** data ;
    unsigned long long *data_len ;
    d_sock = pasvMode(sockCs);
    if (d_sock == -1) {
        printf("pasv error");
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "LIST\r\n");
    send_re = sendCommandToftp( sockCs, bufArr );
    printf("%d\n",send_re);
    if (send_re >= 300 || send_re == 0){
        printf("%s\n",return_bufArr);
        return send_re;
    }
    len=total_len = 0;
    bufArr_len = 512;
    void *re_bufArr = malloc(bufArr_len);//分配了给re_bufArr512的空间
    while ( (len = recv( d_sock, bufArr, 512, 0 )) > 0 )
    {
        if (total_len+len > bufArr_len)
        {
            bufArr_len *= 2;
            void *re_bufArr_n = malloc(bufArr_len);
            memcpy(re_bufArr_n, re_bufArr, total_len);
            free(re_bufArr);
            re_bufArr = re_bufArr_n;
        }
        if (bufArr!=NULL)
            printf("%s\n",bufArr);
        memcpy(re_bufArr+total_len, bufArr, len);
        total_len += len;
    }
    close( d_sock );
    
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum != 226 )
    {
        free(re_bufArr);
        return returnNum;
    }
    
    return 0;
}

// 下载文件
int
ml_get( int sockCs, char *s, double speed )
{
    int     d_sock;
    ssize_t     len,write_len;
    char    bufArr[512];
    int     handle;
    int     returnNum;
    handle = open( s,  O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );//可读写，覆盖，我可读取，我可写
    printf("handle %d\n",handle);
    if ( handle == -1 ){
	printf("You have no permission to the file.\n");
	return -1;
    }
    
    d_sock = pasvMode(sockCs);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "RETR %s\r\n", s );
    returnNum = sendCommandToftp( sockCs, bufArr );
    printf("%d\n",returnNum);
    if (returnNum >= 300 || returnNum == 0)
    {
        printf("Sorry, the file does not exist.\n");
        close(handle);
        return returnNum;
    }
    
    bzero(bufArr, sizeof(bufArr));
    int buf_speed=ceil(ECHOMAX*speed);
    while ( (len = recv( d_sock, bufArr, buf_speed, 0 )) > 0 ) {
        write_len = write( handle, bufArr, len );
        if (write_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
        
    }
    close( d_sock );
    close( handle );
    
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum >= 300 ) {
        return returnNum;
    }
    printf("File transmission completed.\n");
    return 0;
}

// 断点下载文件
int
ml_bpGet( int sockCs, char *s, int bpp)
{
    int     d_sock;
    ssize_t     len,write_len;
    char    bufArr[512];
    int     handle;
    int     returnNum;
    handle = open( s,  O_RDWR|O_CREAT|O_APPEND, S_IREAD|S_IWRITE );//可读写，覆盖，我可读取，我可写
    printf("handle %d\n",handle);
    if ( handle == -1 ){
	printf("You have no permission to the file.\n");
	return -1;
    }
    
    d_sock = pasvMode(sockCs);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "REST %d\r\n", bpp);
    printf("breakpoint:%d\n",bpp);
    returnNum = sendCommandToftp( sockCs, bufArr );
    if (returnNum !=350)
    {
        printf("The server does not support breakpoint send.\n");
        return returnNum;
    }
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "RETR %s\r\n", s );
    printf("filename:%s\n",s);
    returnNum = sendCommandToftp( sockCs, bufArr );
    printf("%d\n",returnNum);
    if (returnNum >= 300 || returnNum == 0)
    {
	if(strcmp(type,"I")!=0){
        printf("You are not in binary mode, please transfer to binary mode first\n");
	printf("returnNum:%d\n",returnNum);
	close(handle);
	return returnNum;
	}else{
        printf("Sorry, the file does not exist.\n");
	printf("returnNum:%d\n",returnNum);
        close(handle);
        return returnNum;
	}
    }
    
    bzero(bufArr, sizeof(bufArr));
    while ( (len = recv( d_sock, bufArr, ECHOMAX, 0 )) > 0 ) {
        write_len = write( handle, bufArr, len );
        if (write_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
        
    }
    close( d_sock );
    close( handle );
    
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum >= 300 ) {
        return returnNum;
    }
    printf("File transmission completed.\n");
    return 0;
}



// 上传文件
int
ml_put( int sockCs, char *s,double speed)
{
    int     d_sock;
    ssize_t     len,send_len;
    char    bufArr[512];
    int     handle;
    int send_re;
    int returnNum;
    
    handle = open( s,  O_RDONLY);
    if ( handle == -1 ) {
        printf("Sorry, your file does not exist.\n");
        return -1;
    }
    
    d_sock = pasvMode(sockCs);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "STOR %s\r\n", s );
    send_re = sendCommandToftp( sockCs, bufArr );
    if (send_re == 553)
    {
        printf("The file you created has existed, please change the file name.\n");
        close(handle);
        return send_re;
    }
    if ((send_re >300) || (send_re==0))
    {
        printf("You have no permission.\n");
        close(handle);
        return send_re;
    }
    bzero(bufArr, sizeof(bufArr));
    int buf_speed=ceil(ECHOMAX*speed);
    while ( (len = read( handle, bufArr, buf_speed)) > 0)
    {
        send_len = send(d_sock, bufArr, len, 0);
        if (send_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
    }
    close( d_sock );
    close( handle );
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum >= 300 ) {
        return returnNum;
    }
    printf("File transmission completed.\n");
    return 0;
}

// 修改文件名
int
ml_renamefile( int sockCs, char *s, char *d )
{
    char    bufArr[512];
    int     re;
    
    sprintf( bufArr, "RNFR %s\r\n", s );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 350 ) {
        printf("Sorry, the file does not exist.\n");
        return re;
    }
    sprintf( bufArr, "RNTO %s\r\n", d );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 250 ){
        printf("%s\n",return_bufArr);
        return re;
    }
    return 0;
}

// 删除文件
int
ml_deletefile( int sockCs, char *s )
{
    char    bufArr[512];
    int     re;
    sprintf( bufArr, "DELE %s\r\n", s );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re == 550 ){
        printf("The file does not exist\n");
        return re;
    }
    else if ( re == 450 ){
        printf("You have no permission to delete.\n");
        return re;
    }
    printf("delete %s.\n",s);
    return 0;
}

// 退出
int
quit_toFtp( int sockCs)
{
    int re = 0;
    re = sendCommandToftp( sockCs, "QUIT\r\n" );
    close( sockCs );
    return re;
}


int
main(int argc,char **argv){
    
    if (0 != argc-2)
    {
        printf("%s\n","missing <hostname>");
        exit(0);
    }
    
    ftphostName = argv[1];
    
    int csock = cliopen(argv[1]);
    
    printf("230 Login successful.\n");
    printf("-----------------------------------------\n");
    printf("mkdir---Create folder: mkdir <filename>\n");
    printf("ls---Show filename: dir\n");
    printf("cd---Enter file path: cd <filename>\n");
    printf("rename---Change File name: ren <oldName> <newName>\n");
    printf("get---Download file: get <filename> \n");
    printf("getbp---Download file using breakpoint: getbp <filename> <breakpoint>\n");
    printf("put---Upload file: put <filename> \n");
    printf("del---Delete the file: del <filename> \n");
    printf("pwd---Show the path of current directory: pwd\n");
    printf("ctype---Change the transfer mode: ctype <binary:I/ascii:A>\n");
    printf("help---Help information: help \n");
    printf("quit---exit: quit \n");
    printf("-----------------------------------------\n");
    
    char command[2000] ;
    for(;;){
        printf("ftp(%s)>",ftphostName);
        scanf("%s",command);
        
        if(strcmp(command,"mkdir")==0){
	    printf("Please specify the directory name:");
            scanf("%s",command);
            ml_mkd(csock,command);
	}else if(strcmp(command,"ls")==0){
            ml_list_active(csock);
        }else if(strcmp(command,"cd")==0){
	    printf("Please specify the path:");
            scanf("%s",command);
            ml_cwd(csock,command);
        }else if(strcmp(command,"rename")==0){
	    printf("Please specify the old file name:");
            scanf("%s",command);
            char command2[1000] ;
            printf("Please specify the new file name:");
            scanf("%s",command2);
            ml_renamefile(csock,command,command2);
        }else if(strcmp(command,"cd..")==0){
            ml_cdup(csock);
        }else if(strcmp(command,"quit")==0){
            quit_toFtp(csock);
            return 0 ;
        }else if(strcmp(command,"get")==0){
            printf("Please specify the file name:");
            scanf("%s",command);
	    printf("Please specify the speed:");
            scanf("%lf",&speed);
            ml_get(csock,command,speed);
        }else if(strcmp(command,"getbp")==0){
	    printf("Please specify the file name:");
            scanf("%s",command);
	    printf("Please specify the breakpoint:");
            scanf("%d",&bp);
            ml_bpGet(csock,command,bp);
        }else if(strcmp(command,"put")==0){
	    printf("Please specify the file name:");
            scanf("%s",command);
	    printf("Please specify the speed:");
            scanf("%lf",&speed);
            ml_put(csock,command,speed);
        }else if(strcmp(command,"del")==0){
	    printf("Please specify the file name:");
            scanf("%s",command);
            ml_deletefile(csock,command);
        }else if(strcmp(command,"pwd")==0){
            ftp_pwd(csock);
        }else if(strcmp(command,"ctype")==0){
            bzero(command, sizeof(command));
            printf("Please specify the type(A/I):");
            scanf("%s",command);
            while(strcmp(command,"A")!=0&&strcmp(command,"I")!=0){
                printf("Wrong input, please input again(A/I):");
                scanf("%s",command);
            }
            ml_type(csock,command);
        }else if(strcmp(command,"help")==0){
            printf("230 Login successful.\n");
            printf("-----------------------------------------\n");
            printf("mkdir---Create folder: mkdir <filename>\n");
            printf("ls---Show filename: dir\n");
            printf("cd---Enter file path: cd <filename>\n");
            printf("rename---Change File name: ren <oldName> <newName>\n");
            printf("get---Download file: get <filename> \n");
		printf("getbp---Download file using breakpoint: getbp <filename> <breakpoint>\n");
            printf("put---Upload file: put <filename> \n");

            printf("del---Delete the file: del <filename> \n");
            printf("pwd---Show the path of current directory: pwd\n");
            printf("ctype---Change the transfer mode: ctype <binary:I/ascii:A>\n");
            printf("help---Help information: help \n");
            printf("quit---exit: quit \n");
            printf("-----------------------------------------\n");
        }else{
            printf("-ftp: %s: command not found.\n",command);
        }
        
    }
    return 0 ;
}


