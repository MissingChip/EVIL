
#include "malloc.h"

#include "evil.h"

int update_x(EState* v, EWindowState* x);

int main(int argc, char** argv){
    VkExtent2D extent = {680, 420};
    EWindowState window_state;
    eCreateWindow(&extent, &window_state);
    EState state;
    ezInitialize(&state, &window_state);
    int quit = 0;
    while(!quit){
        quit = update_x(&state, &window_state);
        ezDraw(&state);
    }
    ezDestroyState(&state);
}

int update_x(EState* v, EWindowState* x){
    xcb_generic_event_t* event;
    xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom( x->connection, 1, 12, "WM_PROTOCOLS" );
    xcb_intern_atom_cookie_t  delete_cookie    = xcb_intern_atom( x->connection, 0, 16, "WM_DELETE_WINDOW" );
    xcb_intern_atom_reply_t  *protocols_reply  = xcb_intern_atom_reply( x->connection, protocols_cookie, 0 );
    xcb_intern_atom_reply_t  *delete_reply     = xcb_intern_atom_reply( x->connection, delete_cookie, 0 );
    xcb_change_property( x->connection, XCB_PROP_MODE_REPLACE, x->window, (*protocols_reply).atom, 4, 32, 1, &(*delete_reply).atom );
    free( protocols_reply );
    xcb_configure_notify_event_t *configure_event;
    int quit = 0;
    uint16_t width, height;
    while(event = xcb_poll_for_event(x->connection)){
        switch (event->response_type & 0x7f) {
            case XCB_CONFIGURE_NOTIFY:
                configure_event = (xcb_configure_notify_event_t*)event;
                width = configure_event->width;
                height = configure_event->height;
                if((width > 0 && width != v->extent.width) || (height > 0 && height != v->extent.height)){
                    v->extent.width = width;
                    v->extent.height = height;
                    // TODO should probably do something on window size change
                }
                break;
            case XCB_CLIENT_MESSAGE:
                if( (*(xcb_client_message_event_t*)event).data.data32[0] == (*delete_reply).atom ) {
                    quit = 1;
                }
                break;
            default:
                printf("Unknown case: %x\n", event->response_type & 0x7f);
                break;
        }
        free(event);
    }
    if(delete_reply)
        free(delete_reply);
    return quit;
}