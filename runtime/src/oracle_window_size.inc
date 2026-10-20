/* Main desktop window dimensions, kept separately for each executable. */
static bool window_size_valid(int w,int h){return w>=GB_SCREEN_WIDTH&&w<=16384&&h>=GB_SCREEN_HEIGHT+g_menu_h&&h<=16384;}
static void window_size_path(char* out,size_t size){char* base=sdl_save_prefix_dup();snprintf(out,size,"%swindow-size.cfg",base?base:"");if(base)SDL_free(base);}
static void restore_window_size(void){
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    char path[1024],line[128],extra;window_size_path(path,sizeof(path));
    FILE* f=fopen(path,"r");if(!f)return;
    int w=0,h=0;bool ok=fgets(line,sizeof(line),f)&&sscanf(line,"%d %d %c",&w,&h,&extra)==2;fclose(f);
    if(!ok||!window_size_valid(w,h))return;
    SDL_Rect bounds;int display=SDL_GetWindowDisplayIndex(g_window);if(display<0)display=0;
    if(SDL_GetDisplayUsableBounds(display,&bounds)==0){
        int max_w=bounds.w-16,max_h=bounds.h-48;
        if(max_w<GB_SCREEN_WIDTH||max_h<GB_SCREEN_HEIGHT+g_menu_h)return;
        if(w>max_w)w=max_w;
        if(h>max_h)h=max_h;
    }
    SDL_SetWindowSize(g_window,w,h);
#endif
}
static void remember_window_size(void){
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    if(!g_window)return;
    Uint32 flags=SDL_GetWindowFlags(g_window);if(flags&SDL_WINDOW_MINIMIZED)return;
    int w,h;if(flags&SDL_WINDOW_FULLSCREEN){w=g_windowed_w;h=g_windowed_h;}else SDL_GetWindowSize(g_window,&w,&h);
    if(!window_size_valid(w,h))return;
    char path[1024];window_size_path(path,sizeof(path));FILE* f=fopen(path,"w");if(!f)return;
    fprintf(f,"%d %d\n",w,h);fclose(f);
#endif
}
