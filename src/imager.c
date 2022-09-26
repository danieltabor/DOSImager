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
#include<dos.h>
#include<stdio.h>
#include<string.h>
#include<direct.h>
#include<conio.h>
#include<fcntl.h>
#include<process.h>

void gotoxy(short col, short row) {
    union REGS r;
    r.h.ah = 0x0F;
    int86(0x10,&r,&r);
    
    r.h.ah = 0x02;
    /*r.h.bh = r.h.bh;*/
    r.h.dh = (char)row-1;
    r.h.dl = (char)col-1;
    int86(0x10,&r,&r);
}

void clrscr() {
    union REGS r;
    r.h.ah = 0x06;
    r.h.al = 0;
    r.h.bh = 0x02;
    r.h.ch = 0;
    r.h.cl = 0;
    r.h.dh = 24;
    r.h.dl = 80;
    int86(0x10,&r,&r);
}

void show_message(const char* message) {
        clrscr();
        gotoxy(10,15);
        cprintf("%s\n",message);
        gotoxy(0,0);
        getch();
}

char g_img_path[256];
char* get_img_path(char* name) {
        sprintf(g_img_path,"C:\\images\\%s.img",name);
        return g_img_path;
}
char g_doc_path[256];
char* get_doc_path(char* name) {
        sprintf(g_doc_path,"C:\\images\\%s.doc",name);
        return g_doc_path;
}

char g_md5_path[256];
char* get_md5_path(char* name) {
        sprintf(g_md5_path,"C:\\images\\%s.md5",name);
        return g_md5_path;
}

char g_zip_path[256];
char* get_zip_path(char* name) {
        sprintf(g_zip_path,"C:\\images\\%s.zip",name);
        return g_zip_path;
}

char g_opt_path[256];
char* get_opt_path(char* arg0) {
    int i;
    strcpy(g_opt_path,arg0);
    for( i=strlen(g_opt_path); i>=0; i-- ) {
        if( g_opt_path[i] == '\\' ) {
            g_opt_path[i+1] = 0;
            break;
        }
    }
    strcat(g_opt_path,"imager.opt");
    return g_opt_path;
}

char g_command[256];

int g_opt_pause = 1;
int g_opt_zip = 1;
int g_opt_img = 1;
int g_opt_doc = 0;

void write_options(char* arg0) {
    FILE* fp = fopen(get_opt_path(arg0),"wb");
    char opt;
    if( fp == 0 ) {
        printf("Could not write: %s\n",get_opt_path(arg0));
        getch();
        show_message("Writing options file failed");
    }
    else {
        if( g_opt_pause ) { opt='1'; }
        else { opt='0'; }
        fwrite(&opt,1,1,fp);
        if( g_opt_zip ) { opt='1'; }
        else { opt='0'; }
        fwrite(&opt,1,1,fp);
        if( g_opt_img ) { opt='1'; }
        else { opt='0'; }
        fwrite(&opt,1,1,fp);
        if( g_opt_doc ) { opt='1'; }
        else { opt='0'; }
        fwrite(&opt,1,1,fp);
        fclose(fp);
    }
}

void read_options(char* arg0) {
    FILE* fp = fopen(get_opt_path(arg0),"rb");
    char opt;
    if( fp == 0 ){
        write_options(arg0);
    }
    else {
        fread(&opt,1,1,fp);
        if( opt == '1' ) { g_opt_pause = 1; }
        else { g_opt_pause = 0; }
        fread(&opt,1,1,fp);
        if( opt == '1' ) { g_opt_zip = 1; }
        else { g_opt_zip = 0; }
        fread(&opt,1,1,fp);
        if( opt == '1' ) { g_opt_img = 1; }
        else { g_opt_img = 0; }
        fread(&opt,1,1,fp);
        if( opt == '1' ) { g_opt_doc = 1; }
        else { g_opt_doc = 0; }
        fclose(fp);
    }
}

struct set_opt_t{
    int col;
    int row;
    int* var;
    char* unselected;
    char* selected;
} g_opt[] = {
    {10,11,&g_opt_pause,
    "         Pause Between Stages          ",
    ">>>>>>>> Pause Between Stages <<<<<<<<<"},
    {10,12,&g_opt_zip,
    "       Archive Drive Files in ZIP      ",
    ">>>>>> Archive Drive Files in ZIP <<<<<"},
    {10,13,&g_opt_img,
    "        Read Generic Drive Image       ",
    ">>>>>>> Read Generic Drive Image <<<<<<"},
    {10,14,&g_opt_doc,
    "    Read M-Systems DiskOnChip Image    ",
    ">>> Read M-Systems DiskOnChip Image <<<"},
};
    
void set_options(char* arg0) {
    unsigned int input;
    int i;
    unsigned int selected = 0;
    while( 1 ) {
        clrscr();
        gotoxy(10,9);
        cprintf("     Automated Image Read Options      ");
        gotoxy(10,10);
        cprintf("---------------------------------------");
        for( i=0; i<4; i++ ) {
            gotoxy(g_opt[i].col, g_opt[i].row);
            if( selected == i ) {
                cprintf(g_opt[i].selected);
            }
            else {
                cprintf(g_opt[i].unselected);
            }
            if( *(g_opt[i].var) ) {
                cprintf(" [ On]");
            }
            else {
                cprintf(" [Off]");
            }
        }
        gotoxy(0,0);
        input = getch();
        if( input == 27 /*ESC*/) {
            write_options(arg0);
            break;
        }
        else if( input == 13 /*return*/ ||
                input == 32 /*space*/ ) {
            *(g_opt[selected].var) = 1 - *(g_opt[selected].var);
        }
        else if( input == 72 /*UP*/ ) {
            if( selected == 0 ) {
                selected = 3;
            }
            else {
                selected = selected - 1;
            }
        }
        else if( input == 80 /*DOWN*/ ) {
            selected = (selected + 1)%4;
        }
    }
}

void select_image_display(unsigned int offset, unsigned int selected,
        char* filter_ext, unsigned int* file_count, char* selected_name) {
    DIR* dp;
    struct dirent *ep;
    char* ext;
    unsigned int row;
    unsigned int i;

    *file_count = 0;
    *selected_name = 0;
    dp = opendir("c:\\images");
    if( dp == 0 ) {
        return;
    }
    clrscr();
    if( offset > 0 ) {
        gotoxy(50,6);
        cprintf("-");
    }
    gotoxy(34,4);
    cprintf("Select Image");
    for( row=6, i=0; ;  ) {
        ep=readdir(dp);
        if( ! ep ) {
            if( row < 21 ) {
                row++;
                continue;
            }
            else {
                break;
            }
        }
        if( strcmp(ep->d_name,".") == 0 ) {
            continue;
        }
        else if( strcmp(ep->d_name,"..") == 0 ) {
            continue;
        }
        ext = ep->d_name+strlen(ep->d_name)-3;
        if( strcmp(ext,filter_ext) == 0 ) {
            *file_count = *file_count + 1;
            if( i < offset ) {
                i++;
                continue;
            }
            if( row < 21 ) {
                gotoxy(32,row);
                if( i == selected ) {
                    *(ext-1) = 0;
                    cprintf(">>>>%8s<<<<",ep->d_name);
                    memcpy(selected_name,ep->d_name,9);
                }
                else {
                    *(ext-1) = 0;
                    cprintf("    %8s    ",ep->d_name);
                }
                row++;
            }
            else {
                gotoxy(50,20);
                cprintf("+");
            }
            i++;
        }
    }
    gotoxy(0,0);
    closedir(dp);
}

char select_name[9];
char* select_image(char* filter_ext) {
    unsigned int offset = 0;
    unsigned int selected = 0;
    unsigned int file_count = 0;
    unsigned int input;
    while( 1 ) {
        select_image_display(offset,selected,filter_ext,
            &file_count,select_name);
        if( file_count == 0 ) {
            show_message("No images available");
            return 0;
        }
        input = getch();
        if( input == 27 /*ESC*/) {
            return 0;
        }
        else if( input == 13 /*return*/ || 
            input == 32 /*space*/ ) {
            break;
        }
        else if( input == 72 /*UP*/ ) {
            if( selected != 0 ) {
                selected--;
                if( selected < offset ) {
                    offset = selected;
                }
            }
        }
        else if( input == 80 /*DOWN*/ ) {
            selected++;
            if( selected >= file_count ) {
                selected = file_count-1;
            }
            if( selected >= offset+15 ) {
                offset = selected-14;
            }
        }
    }
    return select_name;
}

#define CANCEL_FUNC -1
#define AUTO_FUNC 0
#define ZIP_FUNC 1
#define RHD_FUNC 2
#define WHD_FUNC 3
#define RDOC_FUNC 4
#define WDOC_FUNC 5
#define OPT_FUNC 6
#define FUNC_COUNT 7

struct select_functions_t{
    int col;
    int row;
    char* unselected;
    char* selected;
} g_select_functions[] = {
    {15,10,
    "          Read Automated Image            ",
    ">>>>>>>>> Read Automated Image <<<<<<<<<<<"},
    {15,11,
    "            ZIP Drive Files               ",
    ">>>>>>>>>>> ZIP Drive Files <<<<<<<<<<<<<<"},
    {15,12,
    "        Read Generic Drive Image          ",
    ">>>>>>> Read Generic Drive Image <<<<<<<<<"},
    {15,13,
    "       Write Generic Drive Image          ",
    ">>>>>> Write Generic Drive Image <<<<<<<<<"},
    {15,14,
    "     Read M-Systems DiskOnChip Image      ",
    ">>>> Read M-Systems DiskOnChip Image <<<<<"},
    {15,15,
    "     Write M-Systems DiskOnChip Image     ",
    ">>>> Write M-Systems DiskOnChip Image <<<<"},
    {15,16,
    "            Imager Options                ",
    ">>>>>>>>>>> Imager Options <<<<<<<<<<<<<<<"}
};
    
int select_function() {
    unsigned int input;
    int i;
    unsigned int selected = 0;
    while( 1 ) {
        clrscr();
        for( i=0; i<FUNC_COUNT; i++ ) {
            gotoxy(g_select_functions[i].col, g_select_functions[i].row);
            if( selected == i ) {
                cprintf(g_select_functions[i].selected);
            }
            else {
                cprintf(g_select_functions[i].unselected);
            }
        }
        gotoxy(0,0);
        input = getch();
        if( input == 27 /*ESC*/) {
            return CANCEL_FUNC;
        }
        else if( input == 13 /*return*/ ||
                input == 32 /*space*/ ) {
            return selected;
        }
        else if( input == 72 /*UP*/ ) {
            if( selected == 0 ) {
                selected = 5;
            }
            else {
                selected = selected - 1;
            }
        }
        else if( input == 80 /*DOWN*/ ) {
            selected = (selected + 1)%FUNC_COUNT;
        }
    }
}

char disk_name[9];
char* get_disk_name() {
    unsigned int input;
    unsigned int row;
    unsigned int name_len = 0;
    disk_name[0] = 0;
    while( 1 ) {
        clrscr();
        gotoxy(17,13);
        cprintf("Enter Disk Name ( 8 Characters ): \"%-8s\"",disk_name);
        input = getch();
        if( (input >= '0' && input <= '9') ||
            (input >= 'A' && input <= 'Z') ||
            (input >= 'a' && input <= 'z') ||
            input == '-' || input == '_' ) {
            if( name_len < 8 ) {
                    disk_name[name_len++] = input;
                    disk_name[name_len] = 0;
            }
         }
         else if( input == 8 /*BackSpace*/ ) {
            if( name_len )  {
                name_len--;
                disk_name[name_len] = 0;
            }
         }
         else if( input == 27 /*ESC*/ ) {
            gotoxy(1,14);
            return 0;
         }
         else if( input == 13 /*RET*/ ) {
            gotoxy(1,14);
            return disk_name;
         }
    }
}

#define PAUSE_OFF 0
#define PAUSE_ON 1

void read_zip(char* name, int pause) {
    sprintf(g_command,"ZIP.EXE -r %s D:\\*.*",get_zip_path(name));
    system(g_command);
    if( pause ) {
        cprintf("\nZIP Done");
        getch();
    }
}

void read_img(char* name, int pause) {
    sprintf(g_command,"HDIMG.EXE D %s",get_img_path(name));
    system(g_command);
    if( pause ) {
        cprintf("\nDrive Image Read Done");
        getch();
    }
}

void read_doc(char* name, int pause) {
    sprintf(g_command,"DOCPMAP.EXE /BR /F:%s",get_doc_path(name));
    system(g_command);
    if( pause ) {
        cprintf("\nM-Systems DiskOnChip Image Read Done");
        getch();
    }
}
     
void read_md5(char* name, int pause) {
    sprintf(g_command,"MD5SUM.EXE %s %s %s>> %s",get_zip_path(name),
            get_img_path(name),get_doc_path(name),get_md5_path(name));
    system(g_command);
    if( pause ) {
        cprintf("\nMD5 Done");
        getch();
    }
}

struct auto_disp_t {
    int col;
    int row;
    int *var;
    char* name;
} g_auto_disp[] = {
    {10,12,&g_opt_zip,"Zip Filesystem"},
    {10,14,&g_opt_img,"Read Generic Drive Image"},
    {10,16,&g_opt_doc,"Read M-Systems DiskOnChip Image"},
    {10,18,0,"Create MD5 Hashes"}
};
void read_auto_display(char* name,int stage) {
    int i;
    clrscr();
    gotoxy(10,10);
    cprintf("Creating Image: %s",name);
    for( i=0; i<4; i++ ) {
        gotoxy(g_auto_disp[i].col,g_auto_disp[i].row);
        if( g_auto_disp[i].var == 0 || *g_auto_disp[i].var ) {
            if( stage < i ) { cprintf("[   ] "); }
            else if( stage == i ) { cprintf("[ - ] "); }
            else {  cprintf("[ * ] "); }
        }
        else{
            cprintf("[n/a] ");
        }
        cprintf(g_auto_disp[i].name);
    }
    gotoxy(1,20);
    printf(" ");
    if( stage > 3 ) {
        cprintf("DONE");
        getch();        
    }
}

void read_auto(char* name) {
    int stage = 0;
    read_auto_display(name,stage);
    if( g_opt_zip ) {
        read_zip(name,g_opt_pause);
    }
    stage++;
    read_auto_display(name,stage);
    if( g_opt_img ) {
        read_img(name,g_opt_pause);
    }
    stage++;
    read_auto_display(name,stage);
    if( g_opt_doc ) {
        read_doc(name,g_opt_pause);
    }
    stage++;
    read_auto_display(name,stage);
    read_md5(name,g_opt_pause);
    stage++;
    read_auto_display(name,stage);
}

int confirm_write() {
    unsigned int input;
    char buf[4];
    buf[0] = 0;
    while( 1 ) {
        clrscr();
        gotoxy(17,13);
        cprintf("Enter \"YES\" to confirm write: \"%-3s\"",buf);
        input = getch();
        if( (input == 'Y'|| input == 'y') && 
                strlen(buf) == 0 ) {
            buf[0] = 'Y';
            buf[1] = 0;
        } else if( (input == 'E' || input == 'e') &&
                strlen(buf) == 1 ) {
            buf[1] = 'E';
            buf[2] = 0;
        } else if( (input == 'S' || input == 's') &&
                strlen(buf) == 2 ) {
            buf[2] = 'S';
            buf[3] = 0;
        } else if( input == 27 /*ESC*/ ) {
            return 0;
        } else if( input == 13 /*RET*/ ) {
            if( strlen(buf) == 3 ) {
                printf("\n");
                return 1;
            } else {
                return 0;
            }
        }
    }
}

void write_img(char* name, int pause) {
    sprintf(g_command,"HDIMG.EXE -w D %s",get_img_path(name));
    system(g_command);
    if( pause ) {
        cprintf("\nDrive Image Write Done");
        getch();
    }
}


void write_doc(char* name,char pause) { 
    sprintf(g_command,"DOCPMAP.EXE /BW /F:%s",get_doc_path(name));
    system(g_command);
    if( pause ) {
        getch();
    }
}

int main(int argc, char** argv) {
    int result;
    char* name;
    int fhandle;
    
    read_options(argv[0]);
    
    while( 1 ) {
        result = select_function();
        if( result == CANCEL_FUNC ) {
            break;
        }
        else if( result == AUTO_FUNC ) {
            name = get_disk_name();
            if( name == 0 ) {
                continue;
            }
            if( ! _dos_open(get_md5_path(name),O_RDONLY,&fhandle) ) {
                show_message("Auto image already exists");
                _dos_close(fhandle);
                continue;
            }
            read_auto(name);
        }
        else if( result == ZIP_FUNC ) {
            name = get_disk_name();
            if( name == 0 ) {
                continue;
            }
            if( ! _dos_open(get_zip_path(name),O_RDONLY,&fhandle) ) {
                show_message("Zip already exists");
                _dos_close(fhandle);
                continue;
            }
            gotoxy(1,17);
            read_zip(name,PAUSE_ON);
        }
        else if( result == RHD_FUNC ) {
            name = get_disk_name();
            if( name == 0 ) {
                continue;
            }
            if( ! _dos_open(get_img_path(name),O_RDONLY,&fhandle) ) {
                show_message("Generic drive image already exists");
                _dos_close(fhandle);
                continue;
            }
            read_img(name,PAUSE_ON);
        }
        else if( result == WHD_FUNC ) {
          name = select_image("IMG");
          if( name ) {
              if( ! confirm_write() ) {
                  continue;
              }
              write_img(name,PAUSE_ON);
          }
        }                       
        else if( result == RDOC_FUNC ) {
            name = get_disk_name();
            if( name == 0 ) {
                continue;
            }
            if( ! _dos_open(get_doc_path(name),O_RDONLY,&fhandle) ) {
                show_message("M-Systems DiskOnChip image already exists");
                _dos_close(fhandle);
                continue;
            }
            read_doc(name,PAUSE_ON);
        }
        else if( result == WDOC_FUNC ) {
            name = select_image("DOC");
            if( name ) {
                if( ! confirm_write() ) {
                    continue;
                }
                write_doc(name,PAUSE_ON);
            }
        }
        else if( result == OPT_FUNC ) {
            set_options(argv[0]);
        }
    }
    return 0;
}
