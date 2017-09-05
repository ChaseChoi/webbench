# Webbench

## 安装

```shell
sudo make
sudo make install
#提示install: /usr/local/man/man1: No such file or directory
#-p 制定路径任一parent不存在都会建立 -v 显示具体创建了哪些文件夹
sudo mkdir -pv /usr/local/man/man1
#成功
sudo make install
```

## 测试

```shell
#获取帮助
webbench 
```

## prerequisite 

### main()

`argc(argument count)`: 记录命令行参数的个数

`argv(argument vector)`: 记录参数的内容

#### argc

```c
//args: argument count
//argv: argument vector

#include <stdlib.h>
#include <stdio.h>
int main(int argc, char const *argv[])
{
	printf("argc = %d\n", argc);
	return 0;
}
```

```shell
#命令本身也算入
$ ./add
argc = 1
$ ./add 1
argc = 2
$ ./add 1 2
argc = 3
$ ./add 1 2 3
argc = 4
```

#### argv

```c
#include <stdlib.h>
#include <stdio.h>
int main(int argc, char const *argv[])
{
	//Before C99, must declare before used in for-loop
	int i;
	int sum;
	printf("argc = %d\n", argc);
	if (argc > 1)
	{
		for (i = 1; i < argc; ++i)
		{	
			printf("argv[%d] = %s\n", i, argv[i]);
			//convert str to int
          	sum += atoi(argv[i]);
		}
		printf("Total = %d\n", sum);
	}
	return 0;
}
```



```shell
$ ./add 3 4 5
argc = 4
argv[1] = 3
argv[2] = 4
argv[3] = 5
Total = 12
```

### volatile 

> `volatile` is used to *prevent the compiler from making useful and desirable optimizations*.

在与`外部接口`有关的情况特别会用到

```c
bool usb_interface_flag = 0;
//编译器优化: 变量始终为0
while(usb_interface_flag == 0)
{
    // execute logic for the scenario where the USB isn't connected 
}
```

声明为`volatile`,告诉编译器该变量可能会改变,别优化!

### getopt_long()

[参考资料](http://www.informit.com/articles/article.aspx?p=175771&seqNum=3) 

用于长参数处理

```c
#include <getopt.h>                                                GLIBC
//the first three arguments are the same as getopt()-> Single-Letter option Options   
   int getopt_long(int argc, char *const argv[],
   const char *optstring,
   const struct option *longopts, int *longindex);
```

> To use `getopt()`, call it repeatedly from a `while` loop until it returns `-1`. Each time that it **finds** a valid option letter, it **returns** that letter. If the option takes an argument, `optarg` is set to point to it.

1. `OPTIND`: the **index** of the first unprocessed argument.

```shell
#sample
#$((...)) 执行计算; shift 后令未处理的参数处在$1
shift $((OPTIND-1))
```

2. `OPTARG`: The **argument** to an option is placed in the variable `OPTARG`.
3. `:`:if a character is followed by a colon, the option takes an argument.

```shell
#"l" and "b" take an argument
option = getopt(argc, argv,"apl:b:")
```

4. `::` if an option letter in `optstring` is followed by ***two* colon** characters, then that option is allowed to have an **optional** option **argument.** 
5. `?`:if an **invalid option** is provided, the option variable is **assigned** the value `?`. This behaviour is **only true** when you **prepend** the list of valid options with `:` to disable the default error handling of invalid options.

```shell
#":ht" silent mode
while getopts ":ht" opt; do
  case ${opt} in
    h ) # process option a
      ;;
    t ) # process option l
      ;;
    \? ) echo "Usage: cmd [-h] [-t]
      ;;
  esac
done
```

>  `Silent mode` it allows you to distinguish between "invalid option" (return `?`) and "missing option argument.(return `:`, rather than `?`)

#### option

```c
struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};
```

1. `name`: the name of the option
2. `has_arg`

| Symbolic constant   | Numeric value | Meaning                               |
| ------------------- | ------------- | ------------------------------------- |
| `no_argument`       | `0`           | The option does not take an argument. |
| `required_argument` | `1`           | The option requires an argument.      |
| `optional_argument` | `2`           | The option's argument is optional.    |

3. `int *flag`: if `NULL`, getup_long() returns the `val`; If not, the var it points to is filled in with the value in `val`
4. `int val`: the value to return

| flag              | return value |
| ----------------- | ------------ |
| `NULL`            | `val`        |
| `&name`(<— `val`) | 0            |

> If this pointer is `NULL`, then `getopt_long()` returns the value in the `val` field of the structure. If it's not `NULL`, the variable it points to is filled in with the value in `val` and `getopt_long()` returns `0`.

#### return value

| Return code | Meaning                                  |
| ----------- | ---------------------------------------- |
| `0`         | `getopt_long()` set a flag as found in the long option table. |
| `1`         | `optarg` points at a plain command-line argument. |
| `'?'`       | Invalid option.                          |
| `':'`       | Missing option argument.                 |
| `x`         | Option character *'x'*.                  |
| `–1`        | End of options.                          |

#### sample

> The last element in the array should have **zeros** for all the values. 

```c
struct option longopts[] = {
   { "all",     no_argument,       & do_all,     1   },
   { "file",    required_argument, NULL,         'f' },
   { "help",    no_argument,       & do_help,    1   },
   { "verbose", no_argument,       & do_verbose, 1   },
   //should be all zeros
   { 0, 0, 0, 0 }
};
```

## 字符串操作

### strcspn()定位

> Scans *str1* for the **first occurrence** of **any** of the characters that are **part** of *str2*, returning the number of characters of *str1* read before this first occurrence.

```c
/* strcspn example */
#include <stdio.h>
#include <string.h>

int main ()
{
  char str[] = "fcba73";
  char keys[] = "1234567890";
  int i;
  i = strcspn (str,keys);
  printf ("The first number in str is at position %d.\n",i+1);
  return 0;
}

//The first number in str is at position 5
```



### strrchr()定位

Returns a pointer to the **last** occurrence of *character* in the C string *str*.

### strstr()定位

```c
char * strstr ( char * str1, const char * str2 

#include <string.h>
#include <stdio.h>
 
int main(void)
{
   char *string1 = "needle in a haystack";
   char *string2 = "haystack";
   char *result;
 
   result = strstr(string1,string2);
   /* Result = a pointer to "haystack" */
   printf("%s\n", result);
}
 
/*****************  Output should be similar to: *****************
haystack
*/
```

Returns a pointer to the **first** occurrence of `str2` in `str1`, or a `null` pointer if *str2* is not part of *str1*.

### strncasecmp()前几位比较

```c
int strncasecmp(const char *string1, const char *string2, size_t count);
```

**Description**

The strncasecmp() function compares up to *count* characters of *string1* and *string2* **without** sensitivity to **case**. All alphabetic characters in *string1* and *string2* are **converted to lowercase** before comparison.

> The strncasecmp() function operates on `null terminated` strings. The string arguments to the function are expected to contain a null character ('\0') marking the end of the string.

| **Value**      | **Meaning**                       |
| -------------- | --------------------------------- |
| Less than 0    | *string1* less than *string2*     |
| 0              | *string1* equivalent to *string2* |
| Greater than 0 | *string1* greater than *string2*  |

### index()/strchr()定位

The `index()` function shall be equivalent to `strchr()`.But for maximum portability, it is recommended to **replace** the function call to *index*().

`strchr()`:Returns a pointer to the **first** occurrence of *character* in the C string *str*.If the *character* is **not found**, the function returns a `null pointer`.

```c
/* strchr example */
#include <stdio.h>
#include <string.h>

int main ()
{
  char str[] = "This is a sample string";
  char * pch;
  printf ("Looking for the 's' character in \"%s\"...\n",str);
  pch=strchr(str,'s');
  while (pch!=NULL)
  {
    printf ("found at %d\n",pch-str+1);
    pch=strchr(pch+1,'s');
  }
  return 0;
}
```

```shell
Looking for the 's' character in "This is a sample string"...
found at 4
found at 7
found at 11
found at 18
```

### strcat()拼接

```c
char *strcat(char *dest, const char *src)
```

The C library function **char \*strcat(char *dest, const char *src)** appends the string pointed to by **src** to the end of the string pointed to by **dest**.

`return value: `This function returns a pointer to the resulting string `dest`.

### bzero清零

The **bzero**() function sets the first *n* bytes of the area starting at *s* to zero

> This function is deprecated (marked as LEGACY in POSIX.1-2001): use `memset(3)` in new programs. POSIX.1-2008 **removes** the specification of **bzero**().

## socket

[tutorial](http://beej.us/net2/html/theory.html) 

[GNU参考](https://www.gnu.org/software/libc/manual/html_node/Socket-Concepts.html#Socket-Concepts)

> For each combination of style and namespace there is a ***default protocol***, which you can request by specifying `0` as the `protocol number`. And that’s what you should **normally** do—use the **default**.

>  So the **correct** thing to do is to use **AF_INET** in your `struct sockaddr_in` and **PF_INET** in your call to `socket()`. 

### namespace

`AF`: Address Format

`PF`: Protocol Family

### sockaddr

`address`:The` name` of a socket is normally called an *address*.

`socket address`: consists of a ` host address` and a `port` on that host. 

The functions `bind` and `getsockname` use the generic data type `struct sockaddr *` to represent **a pointer** to a socket address. 

### sin_family

`AF_INET`:This designates the address format that goes with the Internet namespace. 

`AF_INET6`:This is similar to `AF_INET`, but refers to the IPv6 protocol.

### htons

`htons()` (read: "Host to Network Short").

> `"Network Byte Order"` is also know as "Big-Endian Byte Order"

### inet_addr()

 `inet_addr()` returns the address in `Network Byte Order` already--you don't have to call `htonl()`. returns **-1** on **error** [ *(unsigned)-1* just happens to correspond to the IP address `255.255.255.255` ]

or `aton`:ascii to network 

```c
struct sockaddr_in {
    short int          sin_family;  // Address family
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
}; 


// Internet address (a structure for historical reasons)
struct in_addr {
    unsigned long s_addr; // that's a 32-bit long, or 4 bytes
};

ina.sin_addr.s_addr = inet_addr("10.12.110.57"); 

//alternatively
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int inet_aton(const char *cp, struct in_addr *inp); 
```

### my port？

```c
#include <sys/types.h>
#include <sys/socket.h>

int bind(int sockfd, struct sockaddr *my_addr, int addrlen); 
```

*sockfd* is the socket file descriptor returned by `socket()`. *my_addr* is a pointer to a `struct sockaddr` that contains information about your address, namely, port and IP address.  *addrlen* can be set to `sizeof(struct sockaddr)`.

```c

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MYPORT 3490

main()
{
    int sockfd;
    struct sockaddr_in my_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // do some error checking!

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(MYPORT);     // short, network byte order
    my_addr.sin_addr.s_addr = inet_addr("10.12.110.57");
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    // don't forget your error checking for bind():
    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    .
    .
    . 
```

### connect()

```c

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEST_IP   "10.12.110.57"
#define DEST_PORT 23

main()
{
    int sockfd;
    struct sockaddr_in dest_addr;   // will hold the destination addr

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // do some error checking!

    dest_addr.sin_family = AF_INET;          // host byte order
    dest_addr.sin_port = htons(DEST_PORT);   // short, network byte order
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
    memset(&(dest_addr.sin_zero), '\0', 8);  // zero the rest of the struct

    // don't forget to error check the connect()!
    connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    ...
```



### gethostbyname()

```c

/*
** getip.c -- a hostname lookup demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    struct hostent *h;

    if (argc != 2) {  // error check the command line
        fprintf(stderr,"usage: getip address\n");
        exit(1);
    }

    if ((h=gethostbyname(argv[1])) == NULL) {  // get the host info
        herror("gethostbyname");
        exit(1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));
   
   return 0;
}
```

### shutdown/close

```c
int shutdown(int sockfd, int how);
```

*sockfd* is the socket file descriptor you want to shutdown, and `how` is one of the following:

- **0** -- Further receives are disallowed
- **1** -- Further sends are disallowedh
- **2** -- Further sends and receives are disallowed (like `close()`)

`shutdown()` returns **0** on success, and **-1** on error (with *errno* set accordingly.)

> It's important to note that `shutdown()` **doesn't** actually close the file descriptor--it just changes its usability. To `free` a socket descriptor, you need to use `close()`.

`close()` returns **zero** on success. On error, **-1** is returned, and *errno* is set appropriately.

## signal

### 1.sigaction()

```c
int sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
```

`1.sig`: which `signal` to catch. This can be (probably "should" be) a symbolic name from *signal.h* along the lines of `SIGINT`.

`2.*act`:   has a bunch of fields that you can fill in to control the behavior of the signal handler. 

`3*oact`: Lastly *oact* can be `NULL`, but if not, it returns the *old* signal handler information that was in place before. This is useful if you want to restore the previous signal handler at a later time.

### 2.struct sigaction

| **Signal**   | **Description**                          |
| ------------ | ---------------------------------------- |
| `sa_handler` | The signal handler function (or `SIG_IGN` to ignore the signal) |
| `sa_mask`    | A set of signals to block while this one is being handled |
| `sa_flags`   | Flags to modify the behavior of the handler, or `0` |

### 3.sample

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void sigint_handler(int sig)
{
    write(0, "Ahhh! SIGINT!\n", 14);
}

int main(void)
{
    void sigint_handler(int sig); /* prototype */
    char s[200];
    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; // or SA_RESTART
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Enter a string:\n");

    if (fgets(s, sizeof s, stdin) == NULL)
        perror("fgets");
    else 
        printf("You entered: %s\n", s);

    return 0;
}
```

## HTTP

### version0.9-1.1

In `HTTP 0.9`, the server always closes the connection after sending the response. The client must close its end of the connection after receiving the response.

In `HTTP 1.0`, the server always closes the connection after sending the response **UNLESS** the client sent a `Connection: keep-alive` request header and the server sent a `Connection: keep-alive` response header. If no such response header exists, the client must close its end of the connection after receiving the response.

In `HTTP 1.1`, the server does not close the connection after sending the response **UNLESS** the client sent a `Connection: close` request header, or the server sent a `Connection: close`response header. If such a response header exists, the client must close its end of the connection after receiving the response.

### url

```shell
#protocol                 resource path
http://www.domain.com:1234/path/to/resource?a=b&x=y
#                     port                 query
```

### colon

`URL`中的冒号`':'` 是separator, 不是port专属

### https verbs

+ `GET`: `GET` is the most common HTTP method; it says "give me this resource". 

```shell
GET /path/to/file/index.html HTTP/1.0
```

+ `OPTIONS`:used to retrieve the **server capabilities**. On the client-side, it can be used to modify the request based on what the server can support.
+ `HEAD`:A `HEAD reques`t is just like a `GET request`, except it asks the server to return the response **headers only**, and not the actual resource (i.e. no message body). This is useful to check characteristics of a resource without actually downloading it, thus saving bandwidth. Use `HEAD` when you don't actually need a file's contents.
+ `TRACE` :used to `retrieve the hops` that a request takes to round trip from the server. Each intermediate proxy or gateway would inject its IP or DNS name into the `Via` header field. This can be used for `diagnostic purposes`.

### proxy

> When a client uses a proxy, it typically sends all requests to that proxy, instead of to the servers in the URLs. Requests to a proxy differ from normal requests in one way: in the first line, they use the **complete URL** of the resource being requested, instead of just the path. For example,

```shell
GET http://www.somehost.com/path/file.html HTTP/1.0
```

## fork()

| return   value | description                              |
| -------------- | ---------------------------------------- |
| `-1`:          | If it returns `-1`, something went **wrong**, and no child was created. Use **perror()** to see what happened. You've probably filled the process table—if you turn around you'll see your sysadmin coming at you with a fireaxe. |
| `0`            | If it returns `0`, you are the `child process`. You can get the parent's PID by calling **getppid()**. Of course, you can get your own PID by calling **getpid()**. |
| else:          | Any other value returned by **fork()** means that you're the parent and the value returned is the PID of your child. This is the only way to get the PID of your child, since there is no **getcpid()** call (obviously due to the one-to-many relationship between parents and children.) |

## alarm()

> The *alarm*() function shall cause the system to generate a `SIGALRM` signal for the process after the number of realtime seconds specified by *seconds* have elapsed.

## 疑问

line 438 shutdown()

line430 failed--