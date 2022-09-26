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

void usage(char* cmd) {
  fprintf(stderr,"Usage:\n");
  fprintf(stderr,"%s [-h] [-w] drive_letter image_file",cmd);
  exit(1);
}

int main(int argc, char** argv) {
  union REGS r;
  int i;
  char drive_letter;
  FILE* fp;
  unsigned int head_count = 0;
  unsigned int head_idx = 0;
  unsigned int cyl_count = 0;
  unsigned int cyl_idx = 0;
  unsigned int sec_count = 0;
  unsigned int sec_idx = 0;
  unsigned long block_count;
  unsigned long block_idx;
  char buf[512];
  int errline = 0;
  int read_action = 1;

  /* Process Arguments */
  if( argc < 3 || argc > 4 ) { 
    usage(argv[0]);
  }
   
  for( i=0; i<argc; i++ ) {
    if( (strcmp(argv[i],"-h") == 0) || (strlen(argv[i]) == 0) ) { 
      printf("-h or null\n");
      usage(argv[0]); 
    }
    else if( strcmp(argv[i],"-w") == 0) {
      read_action = 0;    
    }
  }

  drive_letter = argv[argc-2][0];
  if( drive_letter >= 'c' && drive_letter <= 'z' ) {
    drive_letter = 'C' + (drive_letter - 'c');
  } 
  else if( drive_letter < 'C' || drive_letter > 'Z' ) {
    printf("bad drive letter: %c\n",drive_letter);
    usage(argv[0]);
  }

  if( read_action ) {
    fp = fopen(argv[argc-1],"wb");
    if( fp == 0 ) {
      fprintf(stderr,"Unable to open path %s for writing.\n",argv[argc-1]);
      exit(2);
    }
  }
  else {
    fp = fopen(argv[argc-1],"rb");
    if( fp == 0 ) {
      fprintf(stderr,"Unable to open path %s for reading.\n",argv[argc-1]);
      exit(2);
    }
  }
  
  /*Read drive parameters*/
  r.h.ah = 0x08;
  r.h.dl = 0x80 + (drive_letter - 'C');
  r.x.si = 0x0;
  r.x.di = 0x0;
  int86(0x13,&r,&r);

  if( r.x.cflag ) {
    fprintf(stderr,"Failed to read drive parameters for %c\n",drive_letter);
    exit(3);
  }
  head_count = r.h.dh+1;
  cyl_count = (((r.x.cx & 0xFF00) >> 8) | ((r.x.cx & 0x00C0) << 2)) + 1;
  sec_count = r.x.cx & 0x3F;
  block_count = (unsigned long)head_count*(unsigned long)cyl_count*(unsigned long)sec_count; 

  printf("Drive %c C/H/S: %d/%d/%d\n",drive_letter,cyl_count,head_count,sec_count);
  fflush(stdout);

  /* Cycle through sectors */
  block_idx = 0;
  for( cyl_idx = 0; cyl_idx < cyl_count; cyl_idx++ ) {
    for( head_idx = 0; head_idx < head_count; head_idx++ ) {
      for( sec_idx = 1; sec_idx  <= sec_count; sec_idx++ ) {
        if( read_action ) {
          if( sec_idx == 1 ) {
            errline = 0;
            printf("\rReading %06ld / %06ld (%03d%%)",block_idx,block_count,
              (block_idx*100)/block_count);
            fflush(stdout);
          }
          memset(buf,0xFF,512);
          r.h.ah = 0x02;
          r.h.al = 1;
          r.h.dl = 0x80 + (drive_letter - 'C');
          r.h.dh = head_idx;
          r.x.cx = ((cyl_idx & 0x00FF) << 8) | ((cyl_idx & 0x0300)>>2) | (sec_idx & 0x3F);
          r.x.bx = (unsigned short)(&buf);
          int86(0x13,&r,&r);

          if( r.x.cflag ) {
            if( ! errline ) { 
              errline=1;
              fprintf(stderr,"\n"); 
            }
            fprintf(stderr,"Failed to read sector (C/H/S): %d/%d/%d\n",cyl_idx,head_idx,sec_idx);
          }
          if( fwrite(buf,1,512,fp) != 512 ) {
            fprintf(stderr,"Failed to write block %d\n",block_idx);
            exit(5);
          }
        }
        else { /* read_action==0 */
          memset(buf,0xFF,512);
          if( fread(buf,1,512,fp) != 512 ) {
            fprintf(stderr,"Failed to read block %d\n",block_idx);
            exit(5);
          }
          if( sec_idx == 1 ) {
            errline = 0;
            printf("\rWriting %06ld / %06ld (%03d%%)",block_idx,block_count,
              (block_idx*100)/block_count);
            fflush(stdout);
          }
          r.h.ah = 0x03;
          r.h.al = 1;
          r.h.dl = 0x80 + (drive_letter - 'C');
          r.h.dh = head_idx;
          r.x.cx = ((cyl_idx & 0x00FF) << 8) | ((cyl_idx & 0x0300)>>2) | (sec_idx & 0x3F);
          r.x.bx = (unsigned short)(&buf);
          int86(0x13,&r,&r);

          if( r.x.cflag ) {
            if( ! errline ) { 
              errline=1;
              fprintf(stderr,"\n"); 
            }
            fprintf(stderr,"Failed to write sector (C/H/S): %d/%d/%d\n",cyl_idx,head_idx,sec_idx);
          }
        }
        block_idx++;
      }
    }
  }

  /* Final Console Output */
  if( read_action ) {
    printf("\rReading %06ld / %06ld (%03d%%)",block_idx,block_count,
      (block_idx*100)/block_count);
  }
  else {
    printf("\rWriting %06ld / %06ld (%03d%%)",block_idx,block_count,
      (block_idx*100)/block_count);
  }
  printf("\nDONE\n");
  fflush(stdout);
  
  fclose(fp);
  return 0;
}
