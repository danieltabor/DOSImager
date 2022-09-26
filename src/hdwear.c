/*
 * Copyright (c) 2022, Daniel Tabor
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include<stdlib.h>
#include<stdio.h>
#include<io.h>
#include<i86.h>
#include<string.h>

char buf00[512];
char buf55[512];
char bufAA[512];
char bufFF[512];

int main(int argc, char** argv) {
    char* bufcycle[4];
    unsigned long wearloop = 1000000;
    unsigned long i;
    unsigned int bi;
    union REGS r;
    unsigned int head_count = 0;
    unsigned int cyl_count = 0;
    unsigned int sec_count = 0;
    
    bufcycle[0] = buf00;
    bufcycle[1] = buf55;
    bufcycle[2] = bufFF;
    bufcycle[3] = bufAA;
    
    for( i=0; i<512; i++ ) {
        buf00[i] = 0x00;
        buf55[i] = 0x55;
        bufAA[i] = 0xAA;
        bufFF[i] = 0xFF;
    }

    /*Read drive parameters*/
    r.h.ah = 0x08;
    r.h.dl = 0x81;
    r.x.si = 0x0;
    r.x.di = 0x0;
    int86(0x13,&r,&r);

    if( r.x.cflag ) {
        fprintf(stderr,"Failed to read drive parameters\n");
        exit(3);
    }
    head_count = r.h.dh+1;
    cyl_count = (((r.x.cx & 0xFF00) >> 8) | ((r.x.cx & 0x00C0) << 2)) + 1;
    sec_count = r.x.cx & 0x3F;
    
    printf("Drive C/H/S: %d/%d/%d\n",cyl_count,head_count,sec_count);
    printf("Wearing out block C/H/S: %d/%d/%d\n",cyl_count/2,head_count/2,sec_count/2);
    /* Wear out a block in the middle of the drive */
    for( i=0; i<wearloop; i++ ) {
        for( bi=0; bi<4; bi++ ) {
            r.h.ah = 0x03;
            r.h.al = 1;
            r.h.dl = 0x81;
            r.h.dh = (head_count/2);
            r.x.cx = (((cyl_count/2) & 0x00FF) << 8) | (((cyl_count/2) & 0x0300)>>2) | ((sec_count/2) & 0x3F);
            r.x.bx = (unsigned short)(bufcycle[bi]);
            int86(0x13,&r,&r);
            
            if( r.x.cflag ) {
                fprintf(stderr,"\nFailed to write block: %d\n",i);
                exit(4);
            }
        }
        printf("\r %07d / %07d",i,wearloop);
    }
    return 0;
}
