/* Target only the current prefix's main WeCom window's auxiliary title bar. */
typedef unsigned long DWORD;
typedef int BOOL;
typedef void *HANDLE;
typedef struct {long left,top,right,bottom;} RECT;
#define API __declspec(dllimport) __stdcall
API HANDLE FindWindowA(const char *, const char *);
API BOOL EnumWindows(BOOL (__stdcall *)(HANDLE,long),long);
API HANDLE GetParent(HANDLE);
API int GetClassNameA(HANDLE,char *,int);
API DWORD GetWindowThreadProcessId(HANDLE,DWORD *);
API long GetWindowLongA(HANDLE,int);
API long SetWindowLongA(HANDLE,int,long);
API BOOL SetWindowPos(HANDLE,HANDLE,int,int,int,int,unsigned);
API BOOL GetWindowRect(HANDLE,RECT *);
API HANDLE GetStdHandle(DWORD);
API BOOL WriteFile(HANDLE,const void *,DWORD,DWORD *,void *);
API char *GetCommandLineA(void);
API void ExitProcess(DWORD);
API HANDLE CreateMutexA(void *,BOOL,const char *);
API DWORD GetLastError(void);
API HANDLE OpenProcess(DWORD,BOOL,DWORD);
API DWORD WaitForSingleObject(HANDLE,DWORD);
API BOOL CloseHandle(HANDLE);
API void Sleep(DWORD);
static HANDLE main_window;static DWORD main_pid;static int apply,found,changed,watch;
static char output[2048];static unsigned pos;
static void text(const char *s){while(*s&&pos<sizeof(output)-64)output[pos++]=*s++;}
static void hex(DWORD v){text("0x");for(int i=7;i>=0;i--)output[pos++]="0123456789abcdef"[(v>>(4*i))&15];}
static int same(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static BOOL __stdcall inspect(HANDLE w,long unused){
    char cls[128];DWORD pid=0;GetClassNameA(w,cls,128);
    if(!same(cls,"TitleBarWindow")||GetParent(w)!=main_window)return 1;
    GetWindowThreadProcessId(w,&pid);if(pid!=main_pid)return 1;
    DWORD style=GetWindowLongA(w,-16),exstyle=GetWindowLongA(w,-20);RECT before,after;
    GetWindowRect(w,&before);found++;
    if(!watch || !(style&0x00080000)) {text("TitleBarWindow hwnd=");hex((DWORD)w);text(" owner=");hex((DWORD)main_window);text(" style=");hex(style);text(" exstyle=");hex(exstyle);text("\n");}
    if(apply&&!(style&0x00080000)){
        /* WS_SYSMENU makes an owned popup managed in winex11.drv;
           the existing layered/toolwindow flags suppress native decoration. */
        SetWindowLongA(w,-16,style|0x00080000);
        BOOL ok=SetWindowPos(w,0,0,0,0,0,0x37); /* FRAMECHANGED,NOMOVE,NOSIZE,NOZORDER,NOACTIVATE */
        GetWindowRect(w,&after);
        text("new_style=");hex(GetWindowLongA(w,-16));text(" position_refresh=");hex(ok);
        text(" geometry_unchanged=");hex(before.left==after.left&&before.top==after.top&&before.right==after.right&&before.bottom==after.bottom);text("\n");if(ok&&(GetWindowLongA(w,-16)&0x00080000))changed++;
    }
    return 1;
}
static void flush(void){DWORD written;if(pos)WriteFile(GetStdHandle((DWORD)-11),output,pos,&written,0);pos=0;}
void mainCRTStartup(void){
    char *s=GetCommandLineA();
    while(*s){
        if(s[0]=='-'&&s[1]=='a'&&s[2]=='p'&&s[3]=='p'&&s[4]=='l'&&s[5]=='y')apply=1;
        if(s[0]=='-'&&s[1]=='w'&&s[2]=='a'&&s[3]=='t'&&s[4]=='c'&&s[5]=='h'){apply=1;watch=1;}
        s++;
    }
    HANDLE mutex=0,process=0;
    if(watch){
        mutex=CreateMutexA(0,0,"Local\\WeComTitleBarManaged");
        if(!mutex||GetLastError()==183)ExitProcess(0);
        for(unsigned i=0;i<120;i++){
            main_window=FindWindowA("WeWorkWindow",0);
            if(main_window)break;
            Sleep(1000);
        }
    }else main_window=FindWindowA("WeWorkWindow",0);
    if(!main_window){text("WeCom main window not found\n");flush();ExitProcess(1);}
    GetWindowThreadProcessId(main_window,&main_pid);
    if(watch){
        process=OpenProcess(0x00100000,0,main_pid);
        if(!process){text("Cannot watch WeCom process\n");flush();ExitProcess(1);}
    }
    do{
        if(watch){
            HANDLE candidate=FindWindowA("WeWorkWindow",0);DWORD candidate_pid=0;
            if(candidate)GetWindowThreadProcessId(candidate,&candidate_pid);
            if(candidate_pid==main_pid)main_window=candidate;
        }
        found=0;changed=0;EnumWindows(inspect,0);
        if(!watch){text("matched=");hex(found);text(" changed=");hex(changed);text("\n");}
        flush();
        if(!watch)break;
    }while(WaitForSingleObject(process,1000)==258);
    if(process)CloseHandle(process);
    if(mutex)CloseHandle(mutex);
    ExitProcess(watch||found?0:1);
}
