/*********************************************************
 *  Presage, an extensible predictive text entry system
 *  ------------------------------------------------------
 *
 *  Copyright (C) 2008  John Hills 
 *  Copyright (C) 2008  Matteo Vescovi <matteo.vescovi@yahoo.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
                                                                             *
                                                                **********(*)*/

/*
 * A simple GTK presage application that uses the X window system
 * XEvIE extension to intercept key presses and insert user selected
 * predictions into currently focused X windows application provided
 * the X server has a working XEvIE extension enabled.
 * 
 * /etc/X11/xorg.conf requires the following lines to enable XEvIE
 *   Section "Extensions"
 *       Option "XEVIE" "Enable"
 *   EndSection
 *
 *
 * Based on code by Matteo Vescovi and xeviedemo.c from 
 * http://freedesktop.org/wiki/Software/XEvIE
 *
 */

#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/X.h>
#include <X11/extensions/Xevie.h>
#include <X11/Xutil.h>

#include "presage.h"

#include <cstring>
#include <iostream>
#include <sstream>

GtkWindow *topwindow;
GtkWidget *window;
GtkWidget *label;

std::string suggestions;
std::string config;
unsigned int no_suggestions;

Presage presage;

static KeySym get_keysym (XEvent *ev) {
    XKeyEvent *key_ev;
    char buffer[64];
    int bufsize = 63;
    KeySym key;
    XComposeStatus compose;
    int char_count;

    key_ev = (XKeyEvent *)ev;

    char_count = XLookupString(key_ev, buffer, bufsize, &key, &compose);
    buffer[char_count] = '\0';
   
    return(key); 
}

void get_prediction(unsigned int key, char *append_string, int complete_flag){

    std::ostringstream os;
    std::string disp_string;
    std::string prediction;
    static std::vector< std::string > predictions;
    char disp_string_c[BUFSIZ];
    char str[BUFSIZ];
    
    append_string[0] = 0;
 	
    if(complete_flag == TRUE){
    	// check that a prediction is available
    	if((key - XK_F1) < predictions.size()){
            prediction = predictions[key - XK_F1].c_str();
            strcpy(append_string, prediction.c_str());
     
            presage.complete(predictions[key - XK_F1]);
        }
        key = ' ';
    }
	
    str[0] = (char) key;
    str[1] = 0;
    predictions = presage.predict(str);

    for (int i = 0; i < (int) predictions.size(); i++) {
        os << 'F' << i+1 << "   " << predictions[i] << std::endl;
    }

    disp_string = os.str();
    strcpy(disp_string_c, disp_string.c_str());
    
    if(disp_string_c[0] != 0) {
    	gtk_label_set_text(GTK_LABEL(label), disp_string_c );
    }
}

void *presage_thread(void *unused) {
    Display *dpy;
    int major, minor;
    XEvent  event;
    XClientMessageEvent *xcme;
    KeySym key;
    char completion_string_c[BUFSIZ];
    int completion_word_index = 0;
    int completion_flag = FALSE;
    
    if (suggestions.empty()) {
        suggestions = presage.config("Presage.Selector.SUGGESTIONS");
    } 
    else {
        presage.config("Presage.Selector.SUGGESTIONS", suggestions);
    }
    
    no_suggestions = atoi(suggestions.c_str());
    
    dpy = XOpenDisplay(NULL);
    XevieQueryVersion(dpy, &major, &minor);
    printf("major = %d, minor = %d\n", major, minor);
    if(XevieStart(dpy))
     	printf("XevieStart(dpy) finished \n");
    else {
     	printf("XevieStart(dpy) failed, only one client is allowed to do event inte");
     	exit(1);
    }
    
    XevieSelectInput(dpy, KeyPressMask | KeyReleaseMask );

    printf("About to entered while(1)\n");

    while(1) {
  
        printf("In forever loop thread\n");

     	XNextEvent(dpy, &event);
     	xcme = (XClientMessageEvent *)&event;
     	/* for readOnly users, send events back to Xserver immediately */

      	printf("Got XEVIE event\n");
    	
        switch(event.type) {
          case KeyPress:
            key = get_keysym(&event);

            if((isalpha(key) || isspace(key) || ispunct(key) || key == XK_BackSpace) && key < 256){
                XevieSendEvent(dpy, &event, XEVIE_UNMODIFIED);
                get_prediction(tolower(key), completion_string_c, FALSE);
                
                
                if(((key == ',') ||
                    (key == '.') ||
                    (key == '?') ||
                    (key == ';') ||
                    (key == ':')) && 
                   (completion_word_index == 0) && 
                   (completion_flag == TRUE)) {
					
                    event.xkey.keycode = XKeysymToKeycode(dpy, XK_BackSpace);
                    event.type = KeyPress; 
                    XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                    event.type = KeyRelease;
                    XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
           
                    event.type = KeyPress; 
                    XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                    event.type = KeyRelease;
                    XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
                    
                    event.xkey.keycode = XKeysymToKeycode(dpy, key);
                    event.type = KeyPress; 
                    XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                    event.type = KeyRelease;
                    XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
                
                    event.xkey.keycode = XKeysymToKeycode(dpy, XK_space);
                    event.type = KeyPress; 
                    XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                    event.type = KeyRelease;
                    XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
                }

                else if(isspace(key)) completion_word_index = 0;
                else completion_word_index++;
                
                completion_flag = FALSE;
            }
            
            // check for function keys
            else if(((key - XK_F1) >= 0) && ((key - XK_F1) <= no_suggestions-1)) {
            	
            	// inform presage that the prediction was successful.
               	// and ask presage to predict next token
               	get_prediction(tolower(key), completion_string_c, TRUE);
               
                char s[64];
                printf("%d\n", completion_word_index  );
                //  send the rest of the completion string and a ' ' to the client 
                while(completion_string_c[completion_word_index] != '\0'){               	
                    s[0] = completion_string_c[completion_word_index];
                    s[1] = '\0';
                    
                    event.xkey.keycode = XKeysymToKeycode(dpy, XStringToKeysym(s));               
                    event.type = KeyPress; 
                    XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                    event.type = KeyRelease;
                    XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
               		
                    event.xkey.time++;
                          
                    completion_word_index++;
                }
                completion_word_index = 0;
                completion_flag = TRUE;
                
                event.xkey.keycode = XKeysymToKeycode(dpy, XK_space);
                event.type = KeyPress;
                XSendEvent(dpy, InputFocus, TRUE, KeyPressMask, &event);
                event.type = KeyRelease;
                XSendEvent(dpy, InputFocus, TRUE, KeyReleaseMask, &event);
            }
            else {   			
            	// anything else just send it to the client
            	XevieSendEvent(dpy, &event, XEVIE_UNMODIFIED);
                completion_word_index = 0;
                completion_flag = FALSE;
            }
            break;
            
          case KeyRelease:    			
            XevieSendEvent(dpy, &event, XEVIE_UNMODIFIED);
            break;
            
          default: 
            printf(" unknown event %x\n", event.type); 
            break;   
    	}
    }
}


int main( int   argc, char *argv[] ) { 
    pthread_t thread1;


    gtk_rc_parse("~/.presagegtkrc" );
    
    gtk_init (&argc, &argv);
    

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //window = gtk_window_new(GTK_WINDOW_POPUP);
    
    
    gtk_window_set_title(GTK_WINDOW (window), "Presage gtk");
   
   
    gtk_window_set_default_size(GTK_WINDOW(window), 150, 180);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

  	
    label = gtk_label_new("Presage GTK");
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_set_name(label, "presage_suggestions");
  	
    gtk_widget_show  (window); 
    gtk_widget_show  (label);

    gtk_main_iteration_do(FALSE);
    
    /* start a thread  */
    if(pthread_create(&thread1, NULL, presage_thread, NULL) != 0) printf("Can't create thread");

    /* update the window every 5ms */
    while(1){
        gtk_main_iteration_do(FALSE);
        printf(".");
        usleep(5000);
    } 
    
    return 0;
}