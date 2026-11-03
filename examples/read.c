// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/ylplugin.h>
#include <stdio.h>
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    ylp_package *p=NULL; ylp_error e={0}; ylp_buffer b={0};
    if(ylp_open(argv[1],NULL,&p,&e)!=YLP_OK) {fprintf(stderr,"%s\n",e.message);return 1;}
    if(ylp_metadata(p,&b,&e)!=YLP_OK) {ylp_close(p);return 1;}
    fwrite(b.data,1,b.size,stdout);ylp_buffer_free(&b);ylp_close(p);return 0;
}
