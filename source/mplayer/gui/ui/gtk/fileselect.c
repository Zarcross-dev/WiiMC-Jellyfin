/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>

#include "config.h"
#include "gui/ui/gmplayer.h"

#include "gui/ui/pixmaps/up.xpm"
#include "gui/ui/pixmaps/dir.xpm"
#include "gui/ui/pixmaps/file.xpm"

#include "gui/app.h"
#include "gui/interface.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "help_mp.h"
#include "mpcommon.h"
#include "stream/stream.h"
#include "libavutil/common.h"

#include "gui/ui/widgets.h"
#include "fileselect.h"
#include "preferences.h"
#include "tools.h"

#ifndef __linux__
#define get_current_dir_name()  getcwd(NULL, PATH_MAX)
#else
char * get_current_dir_name( void );
#endif

gchar         * fsSelectedFile = NULL;
gchar         * fsSelectedFileUtf8 = NULL;
gchar         * fsSelectedDirectory = NULL;
gchar         * fsSelectedDirectoryUtf8 = NULL;
unsigned char * fsThatDir = ".";
const gchar   * fsFilter = "*";

int             fsType    = 0;

char * fsVideoFilterNames[][2] =
         {
	   { "ASF files (*.asf)",					"*.asf" },
	   { "AVI files (*.avi)",					"*.avi" },
	   { "Autodesk animations (*.fli,*.flc)",			"*.fli,*.flc" },
	   { "DGStation Cuberevo recordings (*.trp)",			"*.trp" },
	   { "DiVX files (*.divx)",					"*.divx" },
	   { "MP3 files (*.mp3,*.mp2)",					"*.mp3,*.mp2" },
	   { "MPEG files (*.mpg,*.mpeg,*.m1v)",				"*.mpg,*.mpeg,*.m1v" },
	   { "Macromedia Flash Video (*.flv)",				"*.flv" },
	   { "Matroska Audio files (*.mka)",				"*.mka" },
	   { "Matroska Media files (*.mkv)",				"*.mkv" },
	   { "NuppelVideo files (*.nuv)",				"*.nuv" },
	   { "OGG Vorbis files (*.ogg)",				"*.ogg" },
	   { "OGG Media files (*.ogm)",					"*.ogm" },
	   { "QuickTime files (*.mov,*.qt,*.mp4)",			"*.mov,*.qt,*.mp4" },
	   { "RealVideo files (*.rm,*.rmvb)",				"*.rm,*.rmvb"  },
	   { "Tivo files (*.ty)",					"*.ty"  },
	   { "VCD/SVCD Images (*.bin)",					"*.bin" },
	   { "VIVO files (*.viv)",					"*.viv" },
	   { "VOB files (*.vob)",					"*.vob" },
	   { "Wave files (*.wav)",					"*.wav" },
	   { "Windows Media Audio (*.wma)",				"*.wma" },
	   { "Windows Media Video (*.wmv)",				"*.wmv" },
	   { "Audio files",						"*.mp2,*.mp3,*.mka,*.ogg,*.wav,*.wma" },
	   { "Video files",						"*.asf,*.avi,*.fli,*.flc,*.trp,*.divx,*.mpg,*.mpeg,*.m1v,*.flv,*.mkv,*.nuv,*.ogm,*.mov,*.qt,*.mp4,*.rm,*.rmvb,*.ty,*.bin,*.viv,*.vob,*.wmv" },
	   { "All files",						"*" },
	   { NULL,NULL }
	 };
int fsLastVideoFilterSelected = -1;

char * fsSubtitleFilterNames[][2] =
         {
           { "AQT (*.aqt)",						"*.aqt" },
           { "ASS (*.ass)",						"*.ass" },
           { "RT  (*.rt) ",						"*.rt"  },
           { "SMI (*.smi)",						"*.smi" },
           { "SRT (*.srt)",						"*.srt" },
           { "SSA (*.ssa)",						"*.ssa" },
           { "SUB (*.sub)",						"*.sub" },
           { "TXT (*.txt)",						"*.txt" },
           { "UTF (*.utf)",						"*.utf" },
           { "Subtitles",						"*.aqt,*.ass,*.rt,*.smi,*.srt,*.ssa,*.sub,*.txt,*.utf" },
           { "All files",						"*" },
	   { NULL,NULL }
	 };
int fsLastSubtitleFilterSelected = -1;

char * fsOtherFilterNames[][2] =
         {
	   { "All files",						"*" },
	   { NULL,NULL }
	 };

char * fsAudioFileNames[][2] =
	 {
	   { "MP3 files (*.mp2, *.mp3)",				"*.mp2,*.mp3" },
	   { "Matroska Audio files (*.mka)",				"*.mka" },
	   { "OGG Vorbis files (*.ogg)",				"*.ogg" },
	   { "WAV files (*.wav)",					"*.wav" },
	   { "WMA files (*.wma)",					"*.wma" },
	   { "Audio files",						"*.mp2,*.mp3,*.mka,*.ogg,*.wav,*.wma" },
	   { "All files",						"*" },
	   { NULL, NULL }
	 };
int fsLastAudioFilterSelected = -1;

char * fsFontFileNames[][2] =
         {
#ifdef CONFIG_FREETYPE
	   { "True Type fonts (*.ttf)",					"*.ttf" },
	   { "Type1 fonts (*.pfb)",					"*.pfb" },
	   { "All fonts",						"*.ttf,*.pfb" },
#else
	   { "Font files (*.desc)",					"*.desc" },
#endif
	   { "All files",						"*" },
	   { NULL,NULL }
	 };
int fsLastFontFilterSelected = -1;

GtkWidget   * fsFileNamesList;
GtkWidget   * fsFNameList;
GtkWidget   * fsFileSelect = NULL;
GdkColormap * fsColorMap;
GtkWidget   * fsOk;
GtkWidget   * fsUp;
GtkWidget   * fsCancel;
GtkWidget   * fsCombo4;
GtkWidget   * fsPathCombo;
GList       * fsList_items = NULL;
GList       * fsTopList_items = NULL;
GtkWidget   * List;
GtkWidget   * fsFilterCombo;

GtkStyle    * style;
GdkPixmap   * dpixmap;
GdkPixmap   * fpixmap;
GdkBitmap   * dmask;
GdkBitmap   * fmask;

static char * get_current_dir_name_utf8( void )
{
 char * dir, * utf8dir;
 dir = get_current_dir_name();
 utf8dir = g_filename_to_utf8( dir, -1, NULL, NULL, NULL );
 if ( !utf8dir ) utf8dir = g_strdup( dir );
 free( dir );
 return utf8dir;
}

static char * Filter( const char * name )
{
 static char tmp[32];
 unsigned int  i,c;
 for ( i=0,c=0;i < strlen( name );i++ )
  {
   if ( ( name[i] >='a' )&&( name[i] <= 'z' ) ) { tmp[c++]='['; tmp[c++]=name[i]; tmp[c++]=name[i] - 32; tmp[c++]=']'; }
    else tmp[c++]=name[i];
  }
 tmp[c]=0;
 return tmp;
}

static void clist_append_fname(GtkWidget * list, char *fname,
                               GdkPixmap *pixmap, GdkPixmap *mask) {
  gint pos;
  gchar *filename, *str[2];
  filename = g_filename_to_utf8(fname, -1, NULL, NULL, NULL);
  str[0] = NULL;
  str[1] = filename ? filename : fname;
  pos = gtk_clist_append(GTK_CLIST(list), str);
  gtk_clist_set_pixmap(GTK_CLIST(list), pos, 0, pixmap, mask);
  g_free(filename);
}

static void CheckDir( GtkWidget * list )
{
 struct stat     fs;
 int             i;
 glob_t          gg;

 if ( !fsFilter[0] ) return;

 gtk_widget_hide( list );
 gtk_clist_clear( GTK_CLIST( list ) );

 clist_append_fname(list, ".",  dpixmap, dmask);
 clist_append_fname(list, "..", dpixmap, dmask);

 glob( "*",0,NULL,&gg );
 for(  i=0;(unsigned)i<gg.gl_pathc;i++ )
  {
   stat( gg.gl_pathv[i],&fs );
   if( !S_ISDIR( fs.st_mode ) ) continue;
   clist_append_fname(list, gg.gl_pathv[i], dpixmap, dmask);
  }
 globfree( &gg );

 if ( strchr( fsFilter,',' ) )
  {
   char tmp[8];
   int  i,c,glob_param = 0;
   for ( i=0,c=0;i<(int)strlen( fsFilter ) + 1;i++,c++ )
    {
     tmp[c]=fsFilter[i];
     if ( ( tmp[c] == ',' )||( tmp[c] == '\0' ) )
      {
       tmp[c]=0; c=-1;
       glob( Filter( tmp ),glob_param,NULL,&gg );
       glob_param=GLOB_APPEND;
      }
    }
  } else glob( Filter( fsFilter ),0,NULL,&gg );

 for(  i=0;(unsigned)i<gg.gl_pathc;i++ )
  {
   stat( gg.gl_pathv[i],&fs );
   if(  S_ISDIR( fs.st_mode ) ) continue;
   clist_append_fname(list, gg.gl_pathv[i], fpixmap, fmask);
  }
 globfree( &gg );

 gtk_clist_set_column_width( GTK_CLIST( list ),0,17 );
 gtk_clist_select_row( GTK_CLIST( list ),0,1 );
 gtk_widget_show( list );
}

void ShowFileSelect( int type,int modal )
{
 int i, k, fsMedium;
 char * tmp = NULL, * dir = NULL;

 if ( fsFileSelect ) gtkActive( fsFileSelect );
  else fsFileSelect=create_FileSelect();

 fsType=type;
 switch ( type )
  {
   case fsVideoSelector:
        gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_FileSelect );
        fsList_items=NULL;
        for( i=0;fsVideoFilterNames[i][0];i++ )
          fsList_items=g_list_append( fsList_items,fsVideoFilterNames[i][0] );
	k = fsLastVideoFilterSelected;
        gtk_combo_set_popdown_strings( GTK_COMBO( List ),fsList_items );
        g_list_free( fsList_items );
        gtk_entry_set_text( GTK_ENTRY( fsFilterCombo ),fsVideoFilterNames[k >= 0 ? k : i-2][0] );
	tmp=guiInfo.Filename;
        break;
   case fsSubtitleSelector:
        gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_SubtitleSelect );
        fsList_items=NULL;
        for( i=0;fsSubtitleFilterNames[i][0];i++ )
          fsList_items=g_list_append( fsList_items,fsSubtitleFilterNames[i][0] );
	k = fsLastSubtitleFilterSelected;
        gtk_combo_set_popdown_strings( GTK_COMBO( List ),fsList_items );
        g_list_free( fsList_items );
        gtk_entry_set_text( GTK_ENTRY( fsFilterCombo ),fsSubtitleFilterNames[k >= 0 ? k : i-2][0] );
	tmp=guiInfo.SubtitleFilename;
        break;
/*   case fsOtherSelector:
        gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_OtherSelect );
        fsList_items=NULL;
        for( i=0;fsOtherFilterNames[i][0];i++ )
          fsList_items=g_list_append( fsList_items,fsOtherFilterNames[i][0] );
        gtk_combo_set_popdown_strings( GTK_COMBO( List ),fsList_items );
        g_list_free( fsList_items );
        gtk_entry_set_text( GTK_ENTRY( fsFilterCombo ),fsOtherFilterNames[0][0] );
	tmp=guiInfo.Othername;
        break;*/
   case fsAudioSelector:
	gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_AudioFileSelect );
	fsList_items=NULL;
	for( i=0;fsAudioFileNames[i][0];i++ )
	  fsList_items=g_list_append( fsList_items,fsAudioFileNames[i][0] );
	k = fsLastAudioFilterSelected;
	gtk_combo_set_popdown_strings( GTK_COMBO( List ),fsList_items );
	g_list_free( fsList_items );
	gtk_entry_set_text( GTK_ENTRY( fsFilterCombo ),fsAudioFileNames[k >= 0 ? k : i-2][0] );
	tmp=guiInfo.AudioFilename;
	break;
   case fsFontSelector:
        gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_FontSelect );
	fsList_items=NULL;
	for( i=0;fsFontFileNames[i][0];i++ )
	  fsList_items=g_list_append( fsList_items,fsFontFileNames[i][0] );
	k = fsLastFontFilterSelected;
	gtk_combo_set_popdown_strings( GTK_COMBO( List ),fsList_items );
	g_list_free( fsList_items );
	gtk_entry_set_text( GTK_ENTRY( fsFilterCombo ),fsFontFileNames[k >= 0 ? k : i-2][0] );
	tmp=font_name;
	break;
  }

 fsMedium=(fsType == fsVideoSelector || fsType == fsSubtitleSelector || fsType == fsAudioSelector);

 if ( !tmp && fsMedium ) tmp=guiInfo.Filename;

 if ( tmp && tmp[0] )
  {
   struct stat f;
   dir = strdup( tmp );

   do
    {
     char * c = strrchr( dir,'/' );
     stat( dir,&f );
     if ( S_ISDIR( f.st_mode ) ) break;
     if ( c ) *c=0;
    } while ( strrchr( dir,'/' ) );

   if ( !dir[0] ) nfree( dir );
  }

 if ( fsTopList_items ) g_list_free( fsTopList_items ); fsTopList_items=NULL;
 {
  unsigned int  i, c = 1;


  if ( fsMedium )
   {
    for ( i=0;i < FF_ARRAY_ELEMS(fsHistory);i++ )
     if ( fsHistory[i] ) { fsTopList_items=g_list_append( fsTopList_items,fsHistory[i] ); if ( c ) c=gstrcmp( dir,fsHistory[i] ); }
   }
  if ( c && dir )
   {
     g_free( fsSelectedDirectoryUtf8 );
     fsSelectedDirectoryUtf8=g_filename_to_utf8( dir, -1, NULL, NULL, NULL );
     fsTopList_items=g_list_prepend( fsTopList_items,fsSelectedDirectoryUtf8 );
   }
 }
 free( dir );
 if ( getenv( "HOME" ) ) fsTopList_items=g_list_append( fsTopList_items,getenv( "HOME" ) );
 fsTopList_items=g_list_append( fsTopList_items,"/home" );
 fsTopList_items=g_list_append( fsTopList_items,"/mnt" );
 fsTopList_items=g_list_append( fsTopList_items,"/" );
 gtk_combo_set_popdown_strings( GTK_COMBO( fsCombo4 ),fsTopList_items );

 gtk_window_set_modal( GTK_WINDOW( fsFileSelect ),modal );

 gtk_widget_show( fsFileSelect );
}

void HideFileSelect( void )
{
 if ( !fsFileSelect ) return;
 gtk_widget_hide( fsFileSelect );
 gtk_widget_destroy( fsFileSelect );
 fsFileSelect=NULL;
}

static void fs_PersistantHistory( char * subject )
{
 unsigned int i;

 if ( fsType != fsVideoSelector ) return;

 for ( i=0;i < FF_ARRAY_ELEMS(fsHistory);i++ )
  if ( fsHistory[i] && !strcmp( fsHistory[i],subject ) )
   {
    char * tmp = fsHistory[i]; fsHistory[i]=fsHistory[0]; fsHistory[0]=tmp;
    return;
   }
 nfree( fsHistory[FF_ARRAY_ELEMS(fsHistory) - 1] );
 for ( i=FF_ARRAY_ELEMS(fsHistory) - 1;i;i-- ) fsHistory[i]=fsHistory[i - 1];
 fsHistory[0]=gstrdup( subject );
}
//-----------------------------------------------

static void fs_fsFilterCombo_activate( GtkEditable * editable,
                                       gpointer user_data )
{
 fsFilter=gtk_entry_get_text( GTK_ENTRY( user_data ) );
 CheckDir( fsFNameList );
}

static void fs_fsFilterCombo_changed( GtkEditable * editable,
                                      gpointer user_data )
{
 const char * str;
 int    i;

 str=gtk_entry_get_text( GTK_ENTRY(user_data ) );

 switch ( fsType )
  {
   case fsVideoSelector:
          for( i=0;fsVideoFilterNames[i][0];i++ )
           if( !strcmp( str,fsVideoFilterNames[i][0] ) )
            { fsFilter=fsVideoFilterNames[i][1]; fsLastVideoFilterSelected = i;	break; }
          break;
   case fsSubtitleSelector:
          for( i=0;fsSubtitleFilterNames[i][0];i++ )
           if( !strcmp( str,fsSubtitleFilterNames[i][0] ) )
            { fsFilter=fsSubtitleFilterNames[i][1]; fsLastSubtitleFilterSelected = i; break; }
          break;
/*   case fsOtherSelector:
          for( i=0;fsOtherFilterNames[i][0];i++ )
           if( !strcmp( str,fsOtherFilterNames[i][0] ) )
            { fsFilter=fsOtherFilterNames[i][1]; break; }
          break;*/
   case fsAudioSelector:
          for( i=0;fsAudioFileNames[i][0];i++ )
           if( !strcmp( str,fsAudioFileNames[i][0] ) )
            { fsFilter=fsAudioFileNames[i][1]; fsLastAudioFilterSelected = i; break; }
	  break;
   case fsFontSelector:
          for( i=0;fsFontFileNames[i][0];i++ )
	    if( !strcmp( str,fsFontFileNames[i][0] ) )
	     { fsFilter=fsFontFileNames[i][1]; fsLastFontFilterSelected = i; break; }
	  break;
   default: return;
  }
 CheckDir( fsFNameList );
}

static void fs_fsPathCombo_activate( GtkEditable * editable,
                                     gpointer user_data )
{
 const unsigned char * str;
 gchar * dirname;

 str=gtk_entry_get_text( GTK_ENTRY( user_data ) );
 dirname = g_filename_from_utf8( str, -1, NULL, NULL, NULL );
 if ( chdir( dirname ? (const unsigned char *)dirname : str ) != -1 ) CheckDir( fsFNameList );
 g_free( dirname );
}

static void fs_fsPathCombo_changed( GtkEditable * editable,
                                    gpointer user_data )
{
 const unsigned char * str;
 gchar * dirname;

 str=gtk_entry_get_text( GTK_ENTRY( user_data ) );
 dirname = g_filename_from_utf8( str, -1, NULL, NULL, NULL );
 if ( chdir( dirname ? (const unsigned char *)dirname : str ) != -1 ) CheckDir( fsFNameList );
 g_free( dirname );
}

static void fs_Up_released( GtkButton * button, gpointer user_data )
{
 chdir( ".." );
 fsSelectedFile=fsThatDir;
 CheckDir( fsFNameList );
 gtk_entry_set_text( GTK_ENTRY( fsPathCombo ),(unsigned char *)get_current_dir_name_utf8() );
 return;
}

static void fs_Ok_released( GtkButton * button, gpointer user_data )
{
 GList         * item;
 int             i = 1;
 struct stat     fs;

 stat( fsSelectedFile,&fs );
 if(  S_ISDIR(fs.st_mode ) )
  {
   chdir( fsSelectedFile );
   fsSelectedFile=fsThatDir;
   CheckDir( fsFNameList );
   gtk_entry_set_text( GTK_ENTRY( fsPathCombo ),(unsigned char *)get_current_dir_name_utf8() );
   return;
  }

        fsSelectedDirectory=(unsigned char *)get_current_dir_name();
 switch ( fsType )
  {
   case fsVideoSelector:
          setddup( &guiInfo.Filename,fsSelectedDirectory,fsSelectedFile );
          guiInfo.StreamType=STREAMTYPE_FILE;
          guiInfo.NewPlay=GUI_FILE_NEW; sub_fps=0;
	  nfree( guiInfo.AudioFilename );
	  nfree( guiInfo.SubtitleFilename );
          fs_PersistantHistory( get_current_dir_name_utf8() );      //totem, write into history
          break;
   case fsSubtitleSelector:
          setddup( &guiInfo.SubtitleFilename,fsSelectedDirectory,fsSelectedFile );
	  mplayerLoadSubtitle( guiInfo.SubtitleFilename );
          break;
/*   case fsOtherSelector:
          setddup( &guiInfo.Othername,fsSelectedDirectory,fsSelectedFile );
          break;*/
   case fsAudioSelector:
          setddup( &guiInfo.AudioFilename,fsSelectedDirectory,fsSelectedFile );
          break;
   case fsFontSelector:
          setddup( &font_name,fsSelectedDirectory,fsSelectedFile );
	  mplayerLoadFont();
	  if ( Preferences ) gtk_entry_set_text( GTK_ENTRY( prEFontName ),font_name );
	  break;
  }

 HideFileSelect();

 item=fsTopList_items;
 while( item )
  {
   if ( !strcmp( item->data,fsSelectedDirectory ) ) i=0;
   item=item->next;
  }
 if ( i ) fsTopList_items=g_list_prepend( fsTopList_items,(gchar *)get_current_dir_name_utf8() );
 if ( uiMainAutoPlay ) { uiMainAutoPlay=0; uiEventHandling( evPlay,0 ); }
  else gui( GUI_SET_STATE,(void *) GUI_STOP );
}

static void fs_Cancel_released( GtkButton * button,gpointer user_data )
{
 HideFileSelect();
 fs_PersistantHistory( get_current_dir_name_utf8() );      //totem, write into history file
}

static void fs_fsFNameList_select_row( GtkWidget * widget, gint row, gint column,
                                       GdkEventButton *bevent, gpointer user_data)
{
 gtk_clist_get_text( GTK_CLIST(widget ),row,1,&fsSelectedFile );
 g_free( fsSelectedFileUtf8 );
 fsSelectedFileUtf8 = g_filename_from_utf8( fsSelectedFile, -1, NULL, NULL, NULL );
 if ( fsSelectedFileUtf8 ) fsSelectedFile = fsSelectedFileUtf8;
 if( bevent && bevent->type == GDK_BUTTON_PRESS )  gtk_button_released( GTK_BUTTON( fsOk ) );
}

static gboolean on_FileSelect_key_release_event( GtkWidget * widget,
                                                 GdkEventKey * event,
                                                 gpointer user_data )
{
 switch ( event->keyval )
  {
   case GDK_Escape:
        gtk_button_released( GTK_BUTTON( fsCancel ) );
        break;
   case GDK_Return:
        gtk_button_released( GTK_BUTTON( fsOk ) );
        break;
   case GDK_BackSpace:
        gtk_button_released( GTK_BUTTON( fsUp ) );
        break;
  }
 return FALSE;
}

static gboolean fs_fsFNameList_event( GtkWidget * widget,
                                      GdkEvent * event,
                                      gpointer user_data )
{
  GdkEventButton *bevent;
  gint row, col;

  (void) user_data;

  bevent = (GdkEventButton *) event;

  if ( event->type == GDK_BUTTON_RELEASE && bevent->button == 2 )
  {
    if ( gtk_clist_get_selection_info( GTK_CLIST( widget ), bevent->x, bevent->y, &row, &col ) )
    {
      gtk_clist_get_text( GTK_CLIST( widget ), row, 1, &fsSelectedFile );
      g_free( fsSelectedFileUtf8 );
      fsSelectedFileUtf8 = g_filename_from_utf8( fsSelectedFile, -1, NULL, NULL, NULL );
      if ( fsSelectedFileUtf8 ) fsSelectedFile = fsSelectedFileUtf8;
      gtk_button_released( GTK_BUTTON( fsOk ) );
      return TRUE;
    }
  }

  return FALSE;
}

static void fs_Destroy( void )
{
 g_free( fsSelectedFileUtf8 );
 fsSelectedFileUtf8 = NULL;
 g_free( fsSelectedDirectoryUtf8 );
 fsSelectedDirectoryUtf8 = NULL;
 WidgetDestroy( fsFileSelect, &fsFileSelect );
}

GtkWidget * create_FileSelect( void )
{
 GtkWidget     * vbox4;
 GtkWidget     * hbox4;
 GtkWidget     * vseparator1;
 GtkWidget     * hbox6;
 GtkWidget     * fsFNameListWindow;
 GtkWidget     * hbuttonbox3;

 GtkWidget     * uppixmapwid;
 GdkPixmap     * uppixmap;
 GdkBitmap     * upmask;
 GtkStyle      * upstyle;


 fsFileSelect=gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_name( fsFileSelect,"fsFileSelect" );
 gtk_object_set_data( GTK_OBJECT( fsFileSelect ),"fsFileSelect",fsFileSelect );
 gtk_widget_set_usize( fsFileSelect,512,300 );
 GTK_WIDGET_SET_FLAGS( fsFileSelect,GTK_CAN_DEFAULT );
 gtk_widget_set_events( fsFileSelect,GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_PROPERTY_CHANGE_MASK | GDK_VISIBILITY_NOTIFY_MASK );
 gtk_window_set_title( GTK_WINDOW( fsFileSelect ),MSGTR_FileSelect );
 gtk_window_set_position( GTK_WINDOW( fsFileSelect ),GTK_WIN_POS_CENTER );
 gtk_window_set_policy( GTK_WINDOW( fsFileSelect ),TRUE,TRUE,TRUE );
 gtk_window_set_wmclass( GTK_WINDOW( fsFileSelect ),"FileSelect","MPlayer" );
 fsColorMap=gdk_colormap_get_system();

 gtk_widget_realize( fsFileSelect );
 gtkAddIcon( fsFileSelect );

 style=gtk_widget_get_style( fsFileSelect );
 dpixmap=gdk_pixmap_colormap_create_from_xpm_d( fsFileSelect->window,fsColorMap,&dmask,&style->bg[GTK_STATE_NORMAL],(gchar **)dir_xpm );
 fpixmap=gdk_pixmap_colormap_create_from_xpm_d( fsFileSelect->window,fsColorMap,&fmask,&style->bg[GTK_STATE_NORMAL],(gchar **)file_xpm );

 vbox4=AddVBox( AddDialogFrame( fsFileSelect ),0 );
 hbox4=AddHBox( vbox4,1 );

 fsCombo4=gtk_combo_new();
 gtk_widget_set_name( fsCombo4,"fsCombo4" );
 gtk_widget_show( fsCombo4 );
 gtk_box_pack_start( GTK_BOX( hbox4 ),fsCombo4,TRUE,TRUE,0 );
 gtk_widget_set_usize( fsCombo4,-2,20 );

 fsPathCombo=GTK_COMBO( fsCombo4 )->entry;
 gtk_widget_set_name( fsPathCombo,"fsPathCombo" );
 gtk_widget_show( fsPathCombo );
 gtk_widget_set_usize( fsPathCombo,-2,20 );

 vseparator1=gtk_vseparator_new();
 gtk_widget_set_name( vseparator1,"vseparator1" );
 gtk_widget_show( vseparator1 );
 gtk_box_pack_start( GTK_BOX( hbox4 ),vseparator1,FALSE,TRUE,0 );
 gtk_widget_set_usize( vseparator1,7,20 );

 upstyle=gtk_widget_get_style( fsFileSelect );
 uppixmap=gdk_pixmap_colormap_create_from_xpm_d( fsFileSelect->window,fsColorMap,&upmask,&upstyle->bg[GTK_STATE_NORMAL],(gchar **)up_xpm );
 uppixmapwid=gtk_pixmap_new( uppixmap,upmask );
 gtk_widget_show( uppixmapwid );

 fsUp=gtk_button_new();
 gtk_container_add( GTK_CONTAINER(fsUp ),uppixmapwid );
 gtk_widget_show( fsUp );
 gtk_box_pack_start( GTK_BOX( hbox4 ),fsUp,FALSE,FALSE,0 );
 gtk_widget_set_usize( fsUp,65,15 );

 AddHSeparator( vbox4 );

 hbox6=AddHBox( NULL,0 );
   gtk_box_pack_start( GTK_BOX( vbox4 ),hbox6,TRUE,TRUE,0 );

 fsFNameListWindow=gtk_scrolled_window_new( NULL,NULL );
 gtk_widget_set_name( fsFNameListWindow,"fsFNameListWindow" );
 gtk_widget_show( fsFNameListWindow );
 gtk_box_pack_start( GTK_BOX( hbox6 ),fsFNameListWindow,TRUE,TRUE,0 );
 gtk_widget_set_usize( fsFNameListWindow,-2,145 );
 gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( fsFNameListWindow ),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC );

 fsFNameList=gtk_clist_new( 2 );
 gtk_widget_set_name( fsFNameList,"fsFNameList" );
 gtk_container_add( GTK_CONTAINER( fsFNameListWindow ),fsFNameList );
 gtk_clist_set_column_width( GTK_CLIST( fsFNameList ),0,80 );
 gtk_clist_set_selection_mode( GTK_CLIST( fsFNameList ),GTK_SELECTION_BROWSE );
 gtk_clist_column_titles_hide( GTK_CLIST( fsFNameList ) );
 gtk_clist_set_shadow_type( GTK_CLIST( fsFNameList ),GTK_SHADOW_ETCHED_OUT );

 AddHSeparator( vbox4 );

 List=gtk_combo_new();
 gtk_widget_set_name( List,"List" );
 gtk_widget_ref( List );
 gtk_object_set_data_full( GTK_OBJECT( fsFileSelect ),"List",List,(GtkDestroyNotify)gtk_widget_unref );
 gtk_widget_show( List );
 gtk_box_pack_start( GTK_BOX( vbox4 ),List,FALSE,FALSE,0 );
 gtk_widget_set_usize( List,-2,20 );

 fsFilterCombo=GTK_COMBO( List )->entry;
 gtk_widget_set_name( fsFilterCombo,"fsFilterCombo" );
 gtk_widget_show( fsFilterCombo );
 gtk_entry_set_editable (GTK_ENTRY( fsFilterCombo ),FALSE );

 AddHSeparator( vbox4 );

 hbuttonbox3=AddHButtonBox( vbox4 );
   gtk_button_box_set_layout( GTK_BUTTON_BOX( hbuttonbox3 ),GTK_BUTTONBOX_END );
   gtk_button_box_set_spacing( GTK_BUTTON_BOX( hbuttonbox3 ),10 );

 fsOk=AddButton( MSGTR_Ok,hbuttonbox3 );
 fsCancel=AddButton( MSGTR_Cancel,hbuttonbox3 );

 gtk_signal_connect( GTK_OBJECT( fsFileSelect ),"destroy",GTK_SIGNAL_FUNC( fs_Destroy ), NULL );
 gtk_signal_connect( GTK_OBJECT( fsFileSelect ),"key_release_event",GTK_SIGNAL_FUNC( on_FileSelect_key_release_event ),NULL );

 gtk_signal_connect( GTK_OBJECT( fsFilterCombo ),"changed",GTK_SIGNAL_FUNC( fs_fsFilterCombo_changed ),fsFilterCombo );
 gtk_signal_connect( GTK_OBJECT( fsFilterCombo ),"activate",GTK_SIGNAL_FUNC( fs_fsFilterCombo_activate ),fsFilterCombo );
 gtk_signal_connect( GTK_OBJECT( fsPathCombo ),"changed",GTK_SIGNAL_FUNC( fs_fsPathCombo_changed ),fsPathCombo );
 gtk_signal_connect( GTK_OBJECT( fsPathCombo ),"activate",GTK_SIGNAL_FUNC( fs_fsPathCombo_activate ),fsPathCombo );
 gtk_signal_connect( GTK_OBJECT( fsUp ),"released",GTK_SIGNAL_FUNC( fs_Up_released ),fsFNameList );
 gtk_signal_connect( GTK_OBJECT( fsOk ),"released",GTK_SIGNAL_FUNC( fs_Ok_released ),fsCombo4 );
 gtk_signal_connect( GTK_OBJECT( fsCancel ),"released",GTK_SIGNAL_FUNC( fs_Cancel_released ),NULL );
 gtk_signal_connect( GTK_OBJECT( fsFNameList ),"select_row",(GtkSignalFunc)fs_fsFNameList_select_row,NULL );
 gtk_signal_connect( GTK_OBJECT( fsFNameList ),"event", (GtkSignalFunc)fs_fsFNameList_event,NULL );

 gtk_widget_grab_focus( fsFNameList );

 return fsFileSelect;
}
