#include <gapplication.h>
#include <gapplication-win32.h>

#include <windows.h>
#include <Shellscalingapi.h>
HWND hwndcopy;
class GGApplicationManager
{
    
public:
    GGApplicationManager()
    {
        gid_ = g_NextId();
    }
    
    ~GGApplicationManager()
    {
        gevent_RemoveEventsWithGid(gid_);
    }
	
	int getScreenDensity(int *ldpi)
    {
#if NTDDI_VERSION >= NTDDI_WINBLUE
		HMONITOR mon=MonitorFromWindow(hwndcopy,MONITOR_DEFAULTTOPRIMARY);
		if (mon) {
			UINT x,y;
			if (GetDpiForMonitor(mon,MDT_RAW_DPI,&x,&y)==0) {
				return x;
			}
		}
#endif
      return 96; //Default ?
    }

	void exit()
	{
		PostQuitMessage(0);
	}

    g_id addCallback(gevent_Callback callback, void *udata)
    {
        return callbackList_.addCallback(callback, udata);
    }
    
    void removeCallback(gevent_Callback callback, void *udata)
    {
        callbackList_.removeCallback(callback, udata);
    }
    
    void removeCallbackWithGid(g_id gid)
    {
        callbackList_.removeCallbackWithGid(gid);
    }
    
    static void callback_s(int type, void *event, void *udata)
    {
        static_cast<GGApplicationManager*>(udata)->callbackList_.dispatchEvent(type, event);
    }
    
    void enqueueEvent(int type, void *event, int free)
    {
        gevent_EnqueueEvent(gid_, callback_s, type, event, free, this);
    }
    
private:
    gevent_CallbackList callbackList_;
    g_id gid_;
};


static GGApplicationManager *s_manager = NULL;

extern "C" {

void gapplication_init()
{
    s_manager = new GGApplicationManager;
}

void gapplication_init_w32(HWND hwnd)
{
    hwndcopy=hwnd;
}

void gapplication_cleanup()
{
    delete s_manager;
    s_manager = NULL;
}

g_id gapplication_addCallback(gevent_Callback callback, void *udata)
{
    return s_manager->addCallback(callback, udata);
}

void gapplication_removeCallback(gevent_Callback callback, void *udata)
{
    s_manager->removeCallback(callback, udata);
}

void gapplication_removeCallbackWithGid(g_id gid)
{
    s_manager->removeCallbackWithGid(gid);
}

void gapplication_exit()
{
	s_manager->exit();
}

int gapplication_getScreenDensity(int *ldpi){
	return s_manager->getScreenDensity(ldpi);
}

void gapplication_enqueueEvent(int type, void *event, int free)
{
    s_manager->enqueueEvent(type, event, free);
}

}
