/* External-ROM entry point, shared by the two isolated test executables. */
#define SDL_MAIN_HANDLED
#include "gbrt.h"
#include "platform_sdl.h"
#include "oracle_rom.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef ORACLE_ROM_SHA1
#error A specific supported ROM SHA-1 is required.
#endif
int main(int argc,char**argv){
    SDL_SetMainReady();
    const char* rom_path=NULL;
    for(int i=1;i<argc;++i){
        if(!strcmp(argv[i],"--rom")&&i+1<argc)rom_path=argv[++i];
        else if(!strcmp(argv[i],"--trace"))gbrt_trace_enabled=true;
        else if(!strcmp(argv[i],"--limit")&&i+1<argc)gbrt_instruction_limit=strtoull(argv[++i],NULL,10);
        else if(!strcmp(argv[i],"--trace-entries")&&i+1<argc)gbrt_set_trace_file(argv[++i]);
        else if(!strcmp(argv[i],"--input")&&i+1<argc)gb_platform_set_input_script(argv[++i]);
        else if(!strcmp(argv[i],"--dump-frames")&&i+1<argc)gb_platform_set_dump_frames(argv[++i]);
        else if(!strcmp(argv[i],"--screenshot-prefix")&&i+1<argc)gb_platform_set_screenshot_prefix(argv[++i]);
        else{fprintf(stderr,"Unknown or incomplete option: %s\n",argv[i]);return 2;}
    }
    if(!rom_path){
        const char* message="This build contains no embedded ROM. Launch with --rom followed by the path to your own supported ROM.";
        fprintf(stderr,"%s\n",message);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,ORACLE_GAME_TITLE,message,NULL);return 2;
    }
    uint8_t* rom=NULL;size_t size=0;char error[1024];
    if(!oracle_read_rom(rom_path,ORACLE_ROM_SHA1,&rom,&size,error,sizeof(error))){
        fprintf(stderr,"%s\n",error);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,ORACLE_GAME_TITLE,error,NULL);return 2;
    }
    GBContext* ctx=gb_context_create(NULL);
    if(!ctx){free(rom);fprintf(stderr,"Failed to create context.\n");return 1;}
    if(!gb_platform_init(3)){free(rom);gb_context_destroy(ctx);return 1;}
    /* Register callbacks before loading, so existing battery saves are read. */
    gb_platform_register_context(ctx);
    if(!gb_context_load_rom(ctx,rom,size)){
        free(rom);gb_context_destroy(ctx);gb_platform_shutdown();return 1;
    }
    free(rom);gb_platform_set_title(ORACLE_GAME_TITLE);
    while(1){
        gb_run_frame(ctx);
        if(!gb_platform_poll_events(ctx))break;
        if(ctx->frame_done){
            const uint32_t* fb=gb_get_framebuffer(ctx);
            if(fb)gb_platform_render_frame(fb);
            gb_reset_frame(ctx);ctx->stopped=0;gb_platform_vsync();
        }
    }
    /* Final battery flush must run while platform callbacks are still live. */
    gb_context_destroy(ctx);gb_platform_shutdown();return 0;
}
