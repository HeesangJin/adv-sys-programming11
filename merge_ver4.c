#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>

// 1048576 = 1024*1024 =1MB
inline void reverse_and_write(int *rsize, char **pfile, int *line, char *head, char **pfileout);

int main(int argc, char *argv[]){
    int ret = 1; //리턴
    int line1 = 0, line2 = 0, lineout = 0; //line count
    struct timeval before, after; //측정 시간 전,후
    int duration;
    
    int fd1, fd2, fdout; //file descriptor: 항상 필요함
    char *file1, *file2, *fileout, *temp_out; //mmap사용할때 사용
    char *pfile1, *pfile2, *pfileout, *cfile; // file1, file2 mmap 사용할 시 pointer
    int rsize1, rsize2, *csize; // file1 , file2 의 남은 size
    int currentFile; // file1 or file2 선택
    
    int flag = PROT_WRITE | PROT_READ;
    char *head; // to reverse
    int filesize1, filesize2;
    struct stat tempbuf1, tempbuf2;

    //params check: leave0
    if (argc != 4) {
        fprintf(stderr, "usage: %s file1 file2 fout\n", argv[0]);
        goto leave0;
    }
    //file descriptor
    if ((fd1 = open(argv[1], O_RDWR|O_CREAT)) < 0) {
        perror(argv[1]);
        goto leave0;
    }
    if ((fd2 = open(argv[2], O_RDWR|O_CREAT)) < 0) {
        perror(argv[2]);
        goto leave1;
    }
    //get file size
    fstat(fd1, &tempbuf1);
    fstat(fd2, &tempbuf2);
    filesize1 = tempbuf1.st_size;
    filesize2 = tempbuf2.st_size;

    printf("size1 : %d, size2 : %d", filesize1, filesize2);

    //fileout initialize
    if ((fdout = open(argv[3], O_RDWR|O_CREAT,0644)) < 0) {
        perror(argv[3]);
        goto leave2;
    }
    write(fdout, NULL, 0);
    ftruncate(fdout, filesize1 + filesize2);
    
    //file - memory mapping
    if((file1 = mmap(0, filesize1 , flag, MAP_SHARED,fd1,0)) == NULL){
        perror("mmap error");
        goto leave3;
    }
    if((file2 = mmap(0, filesize2 , flag, MAP_SHARED,fd2,0)) == NULL){
        perror("mmap error");
        goto leave4;
    }
    if((fileout = mmap(0, filesize1 + filesize2 , flag, MAP_SHARED, fdout, 0)) == NULL){
        perror("mmap error");
        goto leave5;
    }
    
    gettimeofday(&before, NULL); //시간 측정 시작
    
    //file pointer initialize;
    pfile1 = file1; 
    pfile2 = file2;
    pfileout = fileout;
    
    //rsize: remained filesize
    rsize1 = filesize1; 
    rsize2 = filesize2;

    //selected file is file1
    currentFile = 1;
    
    //line`s first index
    head = pfile1;

    while(rsize1 && rsize2){
        if(currentFile == 1){
            reverse_and_write(&rsize1, &pfile1, &line1, head, &pfileout);
            currentFile = 2;
            head = pfile2; //file1이 끝났으므로 file2의 다음 위치를 head로 잡음
        }
        else if(currentFile == 2){
            reverse_and_write(&rsize2, &pfile2, &line2, head, &pfileout);
            currentFile = 1;
            head = pfile1;
        }
    }

    exit(0);
    while(rsize1){
        reverse_and_write(&rsize1, &pfile1, &line1, head, &pfileout);
        head = pfile1;
    }
    while(rsize2){
        reverse_and_write(&rsize2, &pfile2, &line2, head, &pfileout);
        head = pfile2;
    }
    
    gettimeofday(&after, NULL); //시간 측정 종료
    
    lineout = line1 + line2;
    //=====결과 출력(여긴 그대로 냅두면 될듯)
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %d, File2= %d, Total = %d Lines\n", line1, line2, lineout);
    ret = 0;
    //=====
leave6:
    munmap(fileout,filesize1 + filesize2);
leave5:
    munmap(file2,filesize2);
leave4:
    munmap(file1,filesize1);
leave3:
    close(fdout);
leave2:
    close(fd2);
leave1:
    close(fd1);
leave0:
    return ret; 
}



void reverse_and_write(int *rsize, char **pfile, int *line, char *head, char **pfileout){
    char *tail, *index;

    (*rsize)--;
    if((*((*pfile)++)) == '\n'){
        (*line)++; 
        tail = *pfile; //pfile1은 다음 줄의 첫 인덱스 
        index = --tail; //tail, index는 '\n'을 가르킴
        while(1){  //마지막껀 \n이므로 제외해야함
            *((*pfileout)++) = *(--index); //index는 '\n'보다 하나 전부터 index를 하나씩 뒤로 미루면서 값 대입
            if(index == head){ //라인의 첫인덱스면
                *((*pfileout)++) = *tail; //'\n'을 넣음
                break;
            }
        }
    }
}
