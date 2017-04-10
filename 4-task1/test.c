#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>
#include <string.h>

int char_driver; 

void test() { 
    int k; //, i, sum; 
    char s[3]; 
    memset(s, '2', sizeof(s)); 

    printf("test begin!\n");     

    k = lseek(char_driver, 4, SEEK_CUR); 
    printf("lseek = %d\n", k); 

    k = write(char_driver, s, sizeof(s)); 
    printf("written = %d\n", k); 

    k = lseek(char_driver, 0, SEEK_END); 
    printf("lseek = %d\n", k); 

    k = lseek(char_driver, -4, SEEK_END); 
    printf("lseek = %d\n", k); 

    k = lseek(char_driver, -4, -1); 
    printf("lseek = %d\n", k); 
} 

void initial(char i) { 
    char s[10]; 
    memset(s, i, sizeof(s)); 
    write(char_driver, s, sizeof(s));   
    int k = lseek(char_driver, 0, SEEK_SET); 
    printf("lseek = %d\n", k); 
} 

int main(int argc, char **argv) {  
    char_driver = open("/dev/fourmb", O_RDWR); 
    
    if (char_driver == -1) {  
      printf("unable to open lcd");  
      exit(EXIT_FAILURE);  
    }     
    
    initial('1'); 
    test();     
    close(char_driver);  
    
    return 0;  
}