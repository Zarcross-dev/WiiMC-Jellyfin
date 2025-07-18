/****************************************************************************
 * WiiMC
 * Tantric 2009-2011
 *
 * settings.cpp
 * Settings save/load to XML file
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ogcsys.h>
#include <mxml.h>
#include <ogc/conf.h>

#include "wiimc.h"
#include "menu.h"
#include "fileop.h"
#include "filebrowser.h"
#include "input.h"
#include "settings.h"
#include "utils/gettext.h"
#include "utils/mem2_manager.h"

#define SAVEBUFFERSIZE (64*1024)

struct SWiiSettings WiiSettings;
static char * savebuffer = NULL;

LANG languages[LANGUAGE_SIZE] = {
	{ "Default", "", "" },
	{ "Abkhazian", "ab", "abk" },
	{ "Afar", "aa", "aar" },
	{ "Afrikaans", "af", "afr" },
	{ "Albanian", "sq", "sqi" },
	{ "Amharic", "am", "amh" },
	{ "Arabic", "ar", "ara" },
	{ "Aragonese", "an", "arg" },
	{ "Armenian", "hy", "hye" },
	{ "Assamese", "as", "asm" },
	{ "Avestan", "ae", "ave" },
	{ "Aymara", "ay", "aym" },
	{ "Azerbaijani", "az", "aze" },
	{ "Bashkir", "ba", "bak" },
	{ "Basque", "eu", "baq" },
	{ "Bashkir", "ba", "bak" },
	{ "Basque", "eu", "baq" },
	{ "Belarusian", "be", "bel" },
	{ "Bengali", "bn", "ben" },
	{ "Bihari", "bh", "bih" },
	{ "Bislama", "bi", "bis" },
	{ "Bosnian", "bs", "bos" },
	{ "Breton", "br", "bre" },
	{ "Bulgarian", "bg", "bul" },
	{ "Burmese", "my", "mya" },
	{ "Byelorussian", "be", "bel" },
	{ "Cambodian", "km", "khm" },
	{ "Catalan", "ca", "cat" },
	{ "Chinese", "zh", "chi" },
	{ "Corsican", "co", "cos" },
	{ "Ceske", "cs", "cze" },
	{ "Dansk", "da", "dan" },
	{ "Deutsch", "de", "ger" },
	{ "English", "en", "eng" },
	{ "Esperanto", "eo", "epo" },
	{ "Espa�ol", "es", "spa" },
	{ "Estonian", "et", "est" },
	{ "Finnish", "fi", "fin" },
	{ "Fiji", "fj", "fij" },
	{ "Faroese", "fo", "fao" },
	{ "Fran�ais", "fr", "fre" },
	{ "Frisian", "fy", "fry" },
	{ "Galician", "gl", "glg" },
	{ "Georgian", "ka", "geo" },
	{ "Greek", "el", "gre" },
	{ "Greenlandic", "kl", "kal" },
	{ "Guarani", "gn", "grn" },
	{ "Gujarati", "gu", "guj" },
	{ "Hausa", "ha", "hau" },
	{ "Hebrew", "he", "heb" },
	{ "Hindi", "hi", "hin" },
	{ "Hrvatski", "hr", "scr" },
	{ "Indonesian", "id", "ind" },
	{ "Interlingue", "ie", "ile" },
	{ "Inupiak", "ik", "ipk" },
	{ "Irish", "ga", "gle" },
	{ "Islenska", "is", "ice" },
	{ "Italiano", "it", "ita" },
	{ "Inuktitut", "iu", "iku" },
	{ "Japanese", "ja", "jpn" },
	{ "Javanese", "jw", "jav" },
	{ "Kannada", "kn", "kan" },
	{ "Kashmiri", "ks", "kas" },
	{ "Kazakh", "kk", "kaz" },
	{ "Korean", "ko", "kor" },
	{ "Kurdish", "ku", "kur" },
	{ "Kinyarwanda", "rw", "kin" },
	{ "Kirghiz", "ky", "kir" },
	{ "Kirundi", "rn", "run" },
	{ "Latin", "la", "lat" },
	{ "Lingala", "ln", "lin" },
	{ "Laothian", "lo", "lao" },
	{ "Lithuanian", "lt", "lit" },
	{ "Latvian", "lv", "lav" },
	{ "Macedonian", "mk", "mac" },
	{ "Magyar", "hu", "hun" },
	{ "Malagasy", "mg", "mlg" },
	{ "Malay", "ms", "may" },
	{ "Malayalam", "ml", "mal" },
	{ "Maltese", "mt", "mlt" },
	{ "Maori", "mi", "mao" },
	{ "Marathi", "mr", "mar" },
	{ "Moldavian", "mo", "mol" },
	{ "Mongolian", "mn", "mon" },
	{ "Nauru", "na", "nau" },
	{ "Nederlands", "nl", "dut" },
	{ "Nepali", "ne", "nep" },
	{ "Norsk", "no", "nno" },
	{ "Occitan", "oc", "oci" },
	{ "Oriya", "or", "ori" },
	{ "Oromo", "om", "orm" },
	{ "Pashto,", "ps", "pus" },
	{ "Persian", "fa", "per" },
	{ "Polish", "pl", "pol" },
	{ "Portugues", "pt", "por" },
	{ "Panjabi", "pa", "pan" },
	{ "Quechua", "qu", "que" },
	{ "Romanian", "ro", "rum" },
	{ "Russian", "ru", "rus" },
	{ "Sangho", "sg", "sag" },
	{ "Samoan", "sm", "smo" },
	{ "Sanskrit", "sa", "san" },
	{ "Scots", "gd", "sco" },
	{ "Serbian", "sr", "scc" },
	{ "Shona", "sn", "sna" },
	{ "Sinhalese", "si", "sin" },
	{ "Sindhi", "sd", "snd" },
	{ "Slovak", "sk", "slo" },
	{ "Slovenian", "sl", "slv" },
	{ "Somali", "so", "som" },
	{ "Sundanese", "su", "sun" },
	{ "Swahili", "sw", "swa" },
	{ "Swedish", "sv", "swe" },
	{ "Tagalog", "tl", "tgl" },
	{ "Tajik", "tg", "tgk" },
	{ "Tamil", "ta", "tam" },
	{ "Tatar", "tt", "tat" },
	{ "Telugu", "te", "tel" },
	{ "Thai", "th", "tha" },
	{ "Tibetan", "bo", "tib" },
	{ "Tigrinya", "ti", "tir" },
	{ "Tonga", "to", "ton" },
	{ "Tsonga", "ts", "tso" },
	{ "Turkish", "tr", "tur" },
	{ "Turkmen", "tk", "tuk" },
	{ "Twi", "tw", "twi" },
	{ "Uighur", "ug", "uig" },
	{ "Ukrainian", "uk", "ukr" },
	{ "Urdu", "ur", "urd" },
	{ "Uzbek", "uz", "uzb" },
	{ "Vietnamese", "ui", "vie" },
	{ "Volapuk", "vo", "Vol" },
	{ "Welsh", "cy", "wel" },
	{ "Wolof", "wo", "wol" },
	{ "Xhosa", "xh", "xho" },
	{ "Yiddish", "yi", "yid" },
	{ "Yoruba", "yo", "yor" },
	{ "Zhuang", "za", "zha" },
	{ "Zulu", "zu", "zul" }
};

int GetLangIndex(char *lang)
{
	if(lang[0] == 0)
		return 0;

	for(int i=1; i < LANGUAGE_SIZE; i++)
		if(strcmp(lang, languages[i].abbrev) == 0)
			return i;
	return 0;
}

CP codepages[CODEPAGE_SIZE] = {
	{ "", "" },
	{ "utf-8", "UTF-8" },
	{ "ISO-8859-1", "Western European" },
	{ "ISO-8859-2", "Eastern European" },
	{ "ISO-8859-3", "South European" },
	{ "ISO-8859-4", "North European" },
	{ "ISO-8859-5", "Cyrillic alphabets" },
	{ "ISO-8859-6", "Arabic" },
	{ "ISO-8859-7", "Greek" },
	{ "ISO-8859-8", "Hebrew" },
	{ "ISO-8859-9", "Turkish" },
	{ "ISO-8859-10", "Nordic" },
	{ "ISO-8859-11", "Thai" },
	{ "ISO-8859-13", "Baltic and Polish" },
	{ "ISO-8859-14", "Celtic" },
	{ "Windows-1250", "Central/Eastern European" },
	{ "Windows-1251", "Cyrillic alphabets" },
	{ "Windows-1252", "Western European" },
	{ "Windows-1253", "Greek" },
	{ "Windows-1254", "Turkish" },
	{ "Windows-1255", "Hebrew" },
	{ "Windows-1256", "Arabic" },
	{ "Windows-1257", "Baltic" },
	{ "Windows-1258", "Vietnamese" },
	{ "shift_jis", "Japanese (Shift JIS)" },
	{ "gb2312", "Chinese Simplified (GB2312)" },
	{ "big5", "Chinese Traditional (Big5)" },
	{ "cp949", "Korean (CP949)" }
};

/****************************************************************************
 * Prepare settings data
 *
 * This sets up the save buffer for saving.
 ***************************************************************************/
static mxml_node_t *xml = NULL;
static mxml_node_t *data = NULL;
static mxml_node_t *section = NULL;
static mxml_node_t *item = NULL;
static mxml_node_t *elem = NULL;

static char temp[20];

static const char * toStr(int i)
{
	sprintf(temp, "%d", i);
	return temp;
}
static const char * FtoStr(float i)
{
	sprintf(temp, "%.2f", i);
	return temp;
}

static void createXMLSection(const char * name, const char * description)
{
	section = mxmlNewElement(data, "section");
	mxmlElementSetAttr(section, "name", name);
	mxmlElementSetAttr(section, "description", description);
}

static void createXMLSetting(const char * name, const char * description, const char * value)
{
	item = mxmlNewElement(section, "setting");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "value", value);
	mxmlElementSetAttr(item, "description", description);
}

static void createXMLVariable(const char * name, const char * description, const char * value)
{
	elem = mxmlNewElement(item, "variable");
	mxmlElementSetAttr(elem, "name", name);
	mxmlElementSetAttr(elem, "value", value);
	mxmlElementSetAttr(elem, "description", description);
}

static void createXMLSMBShare(int i)
{
	item = mxmlNewElement(section, "smbshare");
	mxmlElementSetAttr(item, "name", toStr(i));

	// create variables
	createXMLVariable("ip", "SMB Share IP", WiiSettings.smbConf[i].ip);
	createXMLVariable("share", "SMB Share Name", WiiSettings.smbConf[i].share);
	createXMLVariable("user", "SMB Share Username", WiiSettings.smbConf[i].user);
	createXMLVariable("pwd", "SMB Share Password", WiiSettings.smbConf[i].pwd);
	createXMLVariable("displayname", "SMB Display Name", WiiSettings.smbConf[i].displayname);
}

static void createXMLFTPSite(int i)
{
	item = mxmlNewElement(section, "ftpsite");
	mxmlElementSetAttr(item, "name", toStr(i));

	// create variables
	createXMLVariable("ip", "FTP IP", WiiSettings.ftpConf[i].ip);
	createXMLVariable("folder", "FTP Name", WiiSettings.ftpConf[i].folder);
	createXMLVariable("user", "FTP Username", WiiSettings.ftpConf[i].user);
	createXMLVariable("pwd", "FTP Password", WiiSettings.ftpConf[i].pwd);
	createXMLVariable("port", "FTP Port", toStr(WiiSettings.ftpConf[i].port));
	createXMLVariable("passive", "FTP Passive Mode", toStr(WiiSettings.ftpConf[i].passive));
	createXMLVariable("displayname", "FTP Display Name", WiiSettings.ftpConf[i].displayname);
}

static const char * XMLSaveCallback(mxml_node_t *node, int where)
{
	const char *name;

	name = node->value.element.name;

	if(where == MXML_WS_BEFORE_CLOSE)
	{
		if(!strcmp(name, "file") || !strcmp(name, "section"))
			return ("\n");
		else if(!strcmp(name, "smbshare") || !strcmp(name, "ftpsite"))
			return ("\n\t");
	}
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if(!strcmp(name, "file"))
			return ("\n");
		else if(!strcmp(name, "section"))
			return ("\n\n");
		else if(!strcmp(name, "setting") || !strcmp(name, "smbshare") || !strcmp(name, "ftpsite"))
			return ("\n\t");
		else if(!strcmp(name, "variable"))
			return ("\n\t\t");
	}
	return (NULL);
}


static int
prepareSettingsData ()
{
	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "app", APPNAME);
	mxmlElementSetAttr(data, "version", APPVERSION);

	// Global
	createXMLSection("Global", "Global Settings");
	createXMLSetting("theme", "Theme", WiiSettings.theme);
	createXMLSetting("language", "Language", toStr(WiiSettings.language));
	createXMLSetting("volume", "Volume", toStr(WiiSettings.volume));
	createXMLSetting("hideExtensions", "Hide filename extensions", toStr(WiiSettings.hideExtensions));
	createXMLSetting("exitAction", "Exit action", toStr(WiiSettings.exitAction));
	createXMLSetting("rumble", "Wiimote rumble", toStr(WiiSettings.rumble));
	createXMLSetting("screensaverDelay", "Screensaver delay", toStr(WiiSettings.screensaverDelay));
	createXMLSetting("inactivityShutdown", "Inactivity shutdown", toStr(WiiSettings.inactivityShutdown));
	createXMLSetting("lockFolders", "Static folders", toStr(WiiSettings.lockFolders));
	createXMLSetting("startArea", "Starting area", toStr(WiiSettings.startArea));
	createXMLSetting("debug", "Debug", toStr(WiiSettings.debug));
	// Videos
	createXMLSection("Videos", "Videos Settings");
	createXMLSetting("videoZoomHor", "Horizontal video zoom", FtoStr(WiiSettings.videoZoomHor));
	createXMLSetting("videoZoomVert", "Vertical video zoom", FtoStr(WiiSettings.videoZoomVert));
	createXMLSetting("videoXshift", "Video X offset", toStr(WiiSettings.videoXshift));
	createXMLSetting("videoYshift", "Video Y offset", toStr(WiiSettings.videoYshift));
	createXMLSetting("frameDropping", "Frame dropping compensation", toStr(WiiSettings.frameDropping));
	createXMLSetting("aspectRatio", "Aspect ratio", FtoStr(WiiSettings.aspectRatio));
	createXMLSetting("cacheFill", "Cache Fill %", toStr(WiiSettings.cacheFill));
	createXMLSetting("audioLanguage", "Audio language", WiiSettings.audioLanguage);
	createXMLSetting("audioDelay", "Audio delay", FtoStr(WiiSettings.audioDelay));
	createXMLSetting("autoResume", "Auto-resume", toStr(WiiSettings.autoResume));
	createXMLSetting("autoPlayNextVideo", "Auto-play next video", toStr(WiiSettings.autoPlayNextVideo));
	createXMLSetting("skipBackward", "Skip backward", toStr(WiiSettings.skipBackward));
	createXMLSetting("skipForward", "Skip forward", toStr(WiiSettings.skipForward));
	createXMLSetting("videosFolder", "Videos folder", WiiSettings.videosFolder);
	// Music
	createXMLSection("Music", "Music Settings");
	createXMLSetting("playOrder", "Play order", toStr(WiiSettings.playOrder));
	createXMLSetting("musicFolder", "Music folder", WiiSettings.musicFolder);
	// Pictures
	createXMLSection("Pictures", "Pictures Settings");
	createXMLSetting("slideshowDelay", "Slideshow delay", toStr(WiiSettings.slideshowDelay));
	createXMLSetting("picturesFolder", "Pictures folder", WiiSettings.picturesFolder);
	// DVD
	createXMLSection("DVD", "DVD Settings");
	createXMLSetting("dvdMenu", "DVD Menu", toStr(WiiSettings.dvdMenu));
	createXMLSetting("dvdDisabled", "DVD Disabled", toStr(WiiSettings.dvdDisabled));
	// Online Media
	createXMLSection("Online Media", "Online Media Settings");
	createXMLSetting("onlineCacheFill", "Online Cache Fill %", toStr(WiiSettings.onlineCacheFill));
	createXMLSetting("youtubeFormat", "YouTube quality", toStr(WiiSettings.youtubeFormat));
	createXMLSetting("onlinemediaFolder", "Online media folder", WiiSettings.onlinemediaFolder);
	// Network
	createXMLSection("Network", "Network Settings");
	for(int i=0; i<MAX_SHARES; i++)
		createXMLSMBShare(i);
	for(int i=0; i<MAX_SHARES; i++)
		createXMLFTPSite(i);
	// Subtitles
	createXMLSection("Subtitles", "Subtitle Settings");
	createXMLSetting("subtitleVisibility", "Subtitle visibility", toStr(WiiSettings.subtitleVisibility));
	createXMLSetting("subtitleDelay", "Subtitle delay", FtoStr(WiiSettings.subtitleDelay));
	createXMLSetting("subtitleLanguage", "Subtitle language", WiiSettings.subtitleLanguage);
	createXMLSetting("subtitleCodepage", "Subtitle codepage", WiiSettings.subtitleCodepage);
	createXMLSetting("subtitleColor", "Subtitle color", WiiSettings.subtitleColor);
	createXMLSetting("subtitleSize", "Subtitle size", FtoStr(WiiSettings.subtitleSize));

	int datasize = mxmlSaveString(xml, (char *)savebuffer, SAVEBUFFERSIZE, XMLSaveCallback);

	mxmlDelete(xml);
	return datasize;
}

/****************************************************************************
 * loadXMLSetting
 *
 * Load XML elements into variables for an individual variable
 ***************************************************************************/

static void loadXMLSetting(char * var, const char * name, int maxsize)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			snprintf(var, maxsize, "%s", tmp);
	}
}
static void loadXMLSetting(int * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atoi(tmp);
	}
}
static void loadXMLSetting(float * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atof(tmp);
	}
}

static void loadXMLVariable(char * var, const char * name, int maxsize)
{
	elem = mxmlFindElement(item, xml, "variable", "name", name, MXML_DESCEND);
	if(elem)
	{
		const char * tmp = mxmlElementGetAttr(elem, "value");
		if(tmp)
			snprintf(var, maxsize, "%s", tmp);
	}
}
static void loadXMLVariable(int * var, const char * name)
{
	elem = mxmlFindElement(item, xml, "variable", "name", name, MXML_DESCEND);
	if(elem)
	{
		const char * tmp = mxmlElementGetAttr(elem, "value");
		if(tmp)
			*var = atoi(tmp);
	}
}

static void loadXMLSMBShare(int i)
{
	item = mxmlFindElement(xml, xml, "smbshare", "name", toStr(i), MXML_DESCEND);

	if(item)
	{
		// find variables
		loadXMLVariable(WiiSettings.smbConf[i].ip, "ip", sizeof(WiiSettings.smbConf[i].ip));
		loadXMLVariable(WiiSettings.smbConf[i].share, "share", sizeof(WiiSettings.smbConf[i].share));
		loadXMLVariable(WiiSettings.smbConf[i].user, "user", sizeof(WiiSettings.smbConf[i].user));
		loadXMLVariable(WiiSettings.smbConf[i].pwd, "pwd", sizeof(WiiSettings.smbConf[i].pwd));
		loadXMLVariable(WiiSettings.smbConf[i].displayname, "displayname", sizeof(WiiSettings.smbConf[i].displayname));
	}
}

static void loadXMLFTPSite(int i)
{
	item = mxmlFindElement(xml, xml, "ftpsite", "name", toStr(i), MXML_DESCEND);

	if(item)
	{
		// find variables
		loadXMLVariable(WiiSettings.ftpConf[i].ip, "ip", sizeof(WiiSettings.ftpConf[i].ip));
		loadXMLVariable(WiiSettings.ftpConf[i].folder, "folder", sizeof(WiiSettings.ftpConf[i].folder));
		loadXMLVariable(WiiSettings.ftpConf[i].user, "user", sizeof(WiiSettings.ftpConf[i].user));
		loadXMLVariable(WiiSettings.ftpConf[i].pwd, "pwd", sizeof(WiiSettings.ftpConf[i].pwd));
		loadXMLVariable(&WiiSettings.ftpConf[i].port, "port");
		loadXMLVariable(&WiiSettings.ftpConf[i].passive, "passive");
		loadXMLVariable(WiiSettings.ftpConf[i].displayname, "displayname", sizeof(WiiSettings.ftpConf[i].displayname));
	}
}

static void RecurseOnlineMedia(mxml_node_t * top, char * path)
{
	mxml_node_t * next;

	next = mxmlFindElement(top, top, "link", NULL, NULL, MXML_DESCEND_FIRST);

	while(next != NULL)
	{
		const char *name = mxmlElementGetAttr(next, "name");
		const char *addr = mxmlElementGetAttr(next, "addr");
		const char *image = mxmlElementGetAttr(next, "image");
		const char *type = mxmlElementGetAttr(next, "type");

		if(name && addr) // this is a link
		{
			BROWSERENTRY *o_entry = AddEntryOnlineMedia();
			if(!o_entry)
				break;

			o_entry->file = mem2_strdup(path, MEM2_BROWSER);
			if(!o_entry->file) // no mem
			{
				DeleteEntryOnlineMedia(o_entry);
				break;
			}
			
			o_entry->url = mem2_strdup(addr, MEM2_BROWSER);
			if(!o_entry->url) // no mem
			{
				DeleteEntryOnlineMedia(o_entry);
				break;
			}
			
			o_entry->display = mem2_strdup(name, MEM2_BROWSER);
			if(!o_entry->display) // no mem
			{
				DeleteEntryOnlineMedia(o_entry);
				break;
			}

			if(image)
			{
				o_entry->image = mem2_strdup(image, MEM2_BROWSER);
				if(!o_entry->image) // no mem
				{
					DeleteEntryOnlineMedia(o_entry);
					break;
				}
			}
			if(type)
			{
				if(strncmp(type, "search", 6) == 0)
					o_entry->type = TYPE_SEARCH;
				else if(strncmp(type, "playlist", 8) == 0)
					o_entry->type = TYPE_PLAYLIST;
				else
					o_entry->type = TYPE_FILE;
			}
			else
			{
				char ext[7];
				GetExt(o_entry->url, ext);

				if(IsPlaylistExt(ext))
					o_entry->type = TYPE_PLAYLIST;
				else
					o_entry->type = TYPE_FILE;
			}
		}
		next = mxmlFindElement(next, top, "link", NULL, NULL, MXML_NO_DESCEND);
	}

	next = mxmlFindElement(top, top, "folder", NULL, NULL, MXML_DESCEND_FIRST);

	while(next != NULL)
	{
		const char * name = mxmlElementGetAttr(next, "name");
		
		if(name) // this is a folder
		{
			char *newpath;
			newpath = (char*) malloc((MAXPATHLEN+1)*sizeof(char));
			snprintf(newpath, MAXPATHLEN, "%s%s/", path, name);
			RecurseOnlineMedia(next, newpath);
			free(newpath);
		}
		next = mxmlFindElement(next, top, "folder", NULL, NULL, MXML_NO_DESCEND);
	}
}

/****************************************************************************
 * Load online media entries from specified file
 ***************************************************************************/
static void LoadOnlineMediaFile(char * filepath)
{
	int offset = 0;

	savebuffer = (char *)mem2_malloc(SAVEBUFFERSIZE, MEM2_OTHER);

	if(!savebuffer)
		return;

	memset(savebuffer, 0, SAVEBUFFERSIZE);
	offset = LoadFile(savebuffer, SAVEBUFFERSIZE, filepath, SILENT);

	if (offset > 0)
	{
		xml = mxmlLoadString(NULL, savebuffer, MXML_TEXT_CALLBACK);

		if(xml)
		{
			data = mxmlFindElement(xml, xml, "file", NULL, NULL, MXML_DESCEND);
			if(data) RecurseOnlineMedia(data, (char *)"");
			mxmlDelete(xml);
		}
	}
	mem2_free(savebuffer, MEM2_OTHER);
}

/****************************************************************************
 * DefaultSettings
 *
 * Sets all the defaults!
 ***************************************************************************/
void DefaultSettings ()
{
	memset(&WiiSettings, 0, sizeof(SWiiSettings));

	// Global
	WiiSettings.theme[0] = 0;
	WiiSettings.language = CONF_GetLanguage();
	WiiSettings.volume = 50;
	WiiSettings.hideExtensions = 1;
	WiiSettings.exitAction = EXIT_AUTO;
	WiiSettings.rumble = 1;
	WiiSettings.sleepTimer = 0;
	WiiSettings.screensaverDelay = 300;
	WiiSettings.inactivityShutdown = 2;
	WiiSettings.lockFolders = 0;
	WiiSettings.startArea = MENU_BROWSE_VIDEOS;
	// Videos
	WiiSettings.videoZoomHor = 1;
	WiiSettings.videoZoomVert = 1;
	WiiSettings.videoXshift = 0;
	WiiSettings.videoYshift = 0;
	WiiSettings.frameDropping = FRAMEDROPPING_DISABLED;
	WiiSettings.aspectRatio = -2;
	WiiSettings.cacheFill = 30;
	WiiSettings.audioLanguage[0] = 0;
	WiiSettings.audioDelay = 0;
	WiiSettings.autoResume = 1;
	WiiSettings.autoPlayNextVideo = 0;
	WiiSettings.skipBackward = 10;
	WiiSettings.skipForward = 30;
	WiiSettings.videosFolder[0] = 0;
	// Music
	WiiSettings.playOrder = PLAY_SINGLE;
	WiiSettings.musicFolder[0] = 0;
	// Pictures
	WiiSettings.slideshowDelay = 5;
	WiiSettings.picturesFolder[0] = 0;
	// DVD
	WiiSettings.dvdMenu = 1;
	WiiSettings.dvdDisabled = 0;
	// Online Media
	WiiSettings.onlineCacheFill = 20;
	WiiSettings.youtubeFormat = 34;
	WiiSettings.onlinemediaFolder[0] = 0;
	// Network
	for(int i=0; i<MAX_SHARES; i++)
	{
		WiiSettings.smbConf[i].displayname[0] = 0;
		WiiSettings.smbConf[i].ip[0] = 0;
		WiiSettings.smbConf[i].share[0] = 0;
		WiiSettings.smbConf[i].user[0] = 0;
		WiiSettings.smbConf[i].pwd[0] = 0;
	
		WiiSettings.ftpConf[i].displayname[0] = 0;
		WiiSettings.ftpConf[i].ip[0] = 0;
		WiiSettings.ftpConf[i].folder[0] = 0;
		WiiSettings.ftpConf[i].user[0] = 0;
		WiiSettings.ftpConf[i].pwd[0] = 0;
		WiiSettings.ftpConf[i].port = 21;
		WiiSettings.ftpConf[i].passive = 0;
	}
	// Subtitles
	WiiSettings.subtitleVisibility = 1;
	WiiSettings.subtitleDelay = 0;
	WiiSettings.subtitleLanguage[0] = 0;
	WiiSettings.subtitleCodepage[0] = 0;
	sprintf(WiiSettings.subtitleColor, "FFFFFF00");
	WiiSettings.subtitleSize = 2;
}

/****************************************************************************
 * FixInvalidSettings
 *
 * Resets invalid settings back to defaults
 ***************************************************************************/
static void FixInvalidSettings()
{
	// Global
	if(WiiSettings.theme[0] != 0 && strcmp(WiiSettings.theme, "blue") != 0 && 
		strcmp(WiiSettings.theme, "green") != 0 && strcmp(WiiSettings.theme, "grey") != 0 && 
		strcmp(WiiSettings.theme, "orange") != 0 && strcmp(WiiSettings.theme, "red") != 0)
		WiiSettings.theme[0] = 0;
	if(WiiSettings.language < 0 || WiiSettings.language > LANG_LENGTH)
		WiiSettings.language = LANG_ENGLISH;
	if(WiiSettings.volume < 0 || WiiSettings.volume > 100)
		WiiSettings.volume = 50;
	if(WiiSettings.hideExtensions != 1 && WiiSettings.hideExtensions != 0)
		WiiSettings.hideExtensions = 1;
	if(WiiSettings.exitAction < 0 || WiiSettings.exitAction > EXIT_LOADER)
		WiiSettings.exitAction = EXIT_AUTO;
	if(WiiSettings.rumble != 1 && WiiSettings.rumble != 0)
		WiiSettings.rumble = 1;
	if(WiiSettings.screensaverDelay < 120 || WiiSettings.screensaverDelay > 3600)
		WiiSettings.screensaverDelay = 300;
	if(WiiSettings.inactivityShutdown < 0 || WiiSettings.inactivityShutdown > 8)
		WiiSettings.inactivityShutdown = 2;
	if(WiiSettings.lockFolders != 1 && WiiSettings.lockFolders != 0)
		WiiSettings.lockFolders = 0;
	if(WiiSettings.startArea < MENU_BROWSE_VIDEOS || WiiSettings.startArea > MENU_BROWSE_ONLINEMEDIA)
		WiiSettings.startArea = MENU_BROWSE_VIDEOS;
	if(WiiSettings.dvdDisabled && WiiSettings.startArea == MENU_DVD)
		WiiSettings.startArea = MENU_BROWSE_VIDEOS;

	// Videos
	if(WiiSettings.videoZoomHor < 0.5 || WiiSettings.videoZoomHor > 1.5)
		WiiSettings.videoZoomHor = 1;
	if(WiiSettings.videoZoomVert < 0.5 || WiiSettings.videoZoomVert > 1.5)
		WiiSettings.videoZoomVert = 1;
	if(WiiSettings.videoXshift < -50 || WiiSettings.videoXshift > 50)
		WiiSettings.videoXshift = 0;
	if(WiiSettings.videoYshift < -50 || WiiSettings.videoYshift > 50)
		WiiSettings.videoYshift = 0;
	if(WiiSettings.frameDropping < 0 || WiiSettings.frameDropping > FRAMEDROPPING_ALWAYS)
		WiiSettings.frameDropping = FRAMEDROPPING_DISABLED;
	if(WiiSettings.aspectRatio <= 0 || WiiSettings.aspectRatio > 2.36)
		WiiSettings.aspectRatio = -2;
	if(WiiSettings.cacheFill < 10 || WiiSettings.cacheFill > 100)
		WiiSettings.cacheFill = 30;

	if(WiiSettings.audioLanguage[0] != 0)
	{
		bool found = false;
		
		for(int i=0; i < LANGUAGE_SIZE; i++)
		{
			if(strcmp(WiiSettings.audioLanguage, languages[i].abbrev) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
			WiiSettings.audioLanguage[0] = 0;
	}

	if(WiiSettings.audioDelay < -2 || WiiSettings.audioDelay > 2)
		WiiSettings.audioDelay = 0;
	if(WiiSettings.autoResume != 1 && WiiSettings.autoResume != 0)
		WiiSettings.autoResume = 1;
	if(WiiSettings.autoPlayNextVideo != 1 && WiiSettings.autoPlayNextVideo != 0)
		WiiSettings.autoPlayNextVideo = 1;
	if(WiiSettings.skipBackward < 5 || WiiSettings.skipBackward > 1200)
		WiiSettings.skipBackward = 10;
	if(WiiSettings.skipForward < 5 || WiiSettings.skipForward > 1200)
		WiiSettings.skipForward = 30;
	CleanupPath(WiiSettings.videosFolder);

	// Music
	if(WiiSettings.playOrder < 0 || WiiSettings.playOrder > PLAY_LOOP)
		WiiSettings.playOrder = PLAY_SINGLE;
	CleanupPath(WiiSettings.musicFolder);

	// Pictures
	if(WiiSettings.slideshowDelay > 10 || WiiSettings.slideshowDelay < 1)
		WiiSettings.slideshowDelay = 5;
	CleanupPath(WiiSettings.picturesFolder);

	// DVD
	if(WiiSettings.dvdMenu != 0 && WiiSettings.dvdMenu != 1)
		WiiSettings.dvdMenu = 1;
	if(WiiSettings.dvdDisabled != 0 && WiiSettings.dvdDisabled != 1)
		WiiSettings.dvdDisabled = 0;

	// Online Media
	if(WiiSettings.onlineCacheFill < 5 || WiiSettings.onlineCacheFill > 100)
		WiiSettings.onlineCacheFill = 20;
	if(WiiSettings.youtubeFormat != 34 && WiiSettings.youtubeFormat != 18 && WiiSettings.youtubeFormat != 5)
		WiiSettings.youtubeFormat = 34;

	if(!IsOnlineMediaPath(WiiSettings.onlinemediaFolder))
		CleanupPath(WiiSettings.onlinemediaFolder);

	// Network
	for(int i=0; i<MAX_SHARES; i++)
	{
		if(WiiSettings.ftpConf[i].port < 4 || WiiSettings.ftpConf[i].port > 49151)
			WiiSettings.ftpConf[i].port = 21;

		if(WiiSettings.ftpConf[i].passive != 0) // disable PASV support
			WiiSettings.ftpConf[i].passive = 0;
	}
	
	// Subtitles
	if(WiiSettings.subtitleVisibility < 0 || WiiSettings.subtitleVisibility > 1)
		WiiSettings.subtitleVisibility = 1;
	if(WiiSettings.subtitleDelay < -60 || WiiSettings.subtitleDelay > 60)
		WiiSettings.subtitleDelay = 0;

	if(WiiSettings.subtitleLanguage[0] != 0)
	{
		bool found = false;
		
		for(int i=0; i < LANGUAGE_SIZE; i++)
		{
			if(strcmp(WiiSettings.subtitleLanguage, languages[i].abbrev) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
			WiiSettings.subtitleLanguage[0] = 0;
	}

	if(WiiSettings.subtitleCodepage[0] != 0)
	{
		bool found = false;
		for(int i=0; i < CODEPAGE_SIZE; i++)
		{
			if(strcmp(WiiSettings.subtitleCodepage, codepages[i].cpname) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
			WiiSettings.subtitleCodepage[0] = 0;
	}
	
	if(WiiSettings.subtitleColor[0] == 0 || strlen(WiiSettings.subtitleColor) < 8 ||
	(strcmp(WiiSettings.subtitleColor, "00000000") != 0 && 
		strcmp(WiiSettings.subtitleColor, "FFFFFF00") != 0 && 
		strcmp(WiiSettings.subtitleColor, "FFFF0000") != 0 && 
		strcmp(WiiSettings.subtitleColor, "FF000000") != 0))
		sprintf(WiiSettings.subtitleColor, "FFFFFF00");
	
	if(WiiSettings.subtitleSize > 5 || WiiSettings.subtitleSize < 1)
		WiiSettings.subtitleSize = 2.0;
}

/****************************************************************************
 * Save settings
 ***************************************************************************/
bool
SaveSettings (bool silent)
{
	char filepath[1024] = { 0 };
	wchar_t msg[512];
	int datasize;
	int offset = 0;
	DIR *dir = NULL;
	char path[6][MAXPATHLEN];
	int d=0;

	// We'll save using the first available method (probably SD) since this
	// is the method settings will be loaded from by default

	if(appPath[0] != 0)
	{
		sprintf(filepath, "%s/settings.xml", appPath);
	}
	else
	{
		// populate list of potential paths
		if(CheckMount(DEVICE_SD, 1))
			sprintf(path[d++], "sd1:/apps/%s", APPFOLDER);

		for(int m=1; m<6; m++)
		{
			if(!CheckMount(DEVICE_USB, m))
				break;

			sprintf(path[d++], "usb%d:/apps/%s", m, APPFOLDER);
		}

		// no devices found
		if(d == 0)
		{
			if(!silent)
				ErrorPrompt("Could not find a valid SD or USB device - one is required for normal operation.");
			return false;
		}

		// try paths
		for(int i=0; i<d; i++)
		{
			dir = opendir(path[i]);

			if(!dir)
				continue;

			sprintf(filepath, path[i]);
			closedir(dir);
			break;
		}

		// existing path not found - try to create path
		if(filepath[0] == 0)
		{
			if(CheckMount(DEVICE_SD, 1))
				strcpy(filepath, "sd1:");
			else
				strcpy(filepath, "usb1:");

			// ensure the necessary folders exists for saving
			strcat(filepath, "/apps");
			dir = opendir(filepath);
			if (!dir)
			{
				if(mkdir(filepath, 0777) != 0)
				{
					if(!silent)
					{
						swprintf(msg, 512, L"%s %s", gettext("Could not create folder"), filepath);
						ErrorPrompt(msg);
					}
					return false;
				}
			}
			else
			{
				closedir(dir);
			}

			strcat(filepath, "/");
			strcat(filepath, APPFOLDER);
			dir = opendir(filepath);
			if (!dir)
			{
				if(mkdir(filepath, 0777) != 0)
				{
					if(!silent)
					{
						swprintf(msg, 512, L"%s %s", gettext("Could not create folder"), filepath);
						ErrorPrompt(msg);
					}
					return false;
				}
			}
			else
			{
				closedir(dir);
			}
		}
		strcat(filepath, "/settings.xml");
	}

	if (!silent)
		ShowAction ("Saving settings...");

	FixInvalidSettings();
	savebuffer = (char *)mem2_malloc(SAVEBUFFERSIZE, MEM2_OTHER);

	if(savebuffer)
	{
		memset(savebuffer, 0, SAVEBUFFERSIZE);
		datasize = prepareSettingsData();
		offset = SaveFile(savebuffer, filepath, datasize, silent);
		mem2_free(savebuffer, MEM2_OTHER);
	}

	if(!silent)
		CancelAction();

	if (offset > 0)
	{
		strcpy(appPath, filepath); // save successful path
		char * end = strrchr(appPath, '/');
		end[0] = 0; // strip filename

		// save restore points
		sprintf(filepath,"%s/%s",appPath,"restore_points");
		char * buff = wiiSaveRestorePoints(filepath);

		if(buff)
		{
			SaveFile(buff, filepath, strlen(buff), SILENT);
			mem2_free(buff, MEM2_OTHER);
		}
		return true;
	}

	if(!silent)
	{
		swprintf(msg, 512, L"%s %s", gettext("Could not save settings to"), filepath);
		ErrorPrompt(msg);
	}
	return false;
}

/****************************************************************************
 * Load settings from specified file
 ***************************************************************************/
static bool LoadSettingsFile(char * filepath)
{
	bool result = false;
	int offset = 0;

	savebuffer = (char *)mem2_malloc(SAVEBUFFERSIZE, MEM2_OTHER);

	if(!savebuffer)
		return false;

	memset(savebuffer, 0, SAVEBUFFERSIZE);
	offset = LoadFile(savebuffer, SAVEBUFFERSIZE, filepath, SILENT);

	if (offset > 0)
	{
		xml = mxmlLoadString(NULL, savebuffer, MXML_TEXT_CALLBACK);

		if(xml)
		{
			// check settings version
			// we don't do anything with the version #, but we'll store it anyway
			item = mxmlFindElement(xml, xml, "file", "version", NULL, MXML_DESCEND);
			if(item) // a version entry exists
			{
				const char * version = mxmlElementGetAttr(item, "version");

				if(version && strlen(version) == 5)
				{
					// this code assumes version in format X.X.X
					// XX.X.X, X.XX.X, or X.X.XX will NOT work
					int verMajor = version[0] - '0';
					int verMinor = version[2] - '0';
					int verPoint = version[4] - '0';
					int curMajor = APPVERSION[0] - '0';
					int curMinor = APPVERSION[2] - '0';
					int curPoint = APPVERSION[4] - '0';

					// first we'll check that the versioning is valid
					if(!(verMajor >= 0 && verMajor <= 9 &&
						verMinor >= 0 && verMinor <= 9 &&
						verPoint >= 0 && verPoint <= 9))
						result = false;
					else if(verMajor < 1) // less than version 1.0.0
						result = false; // reset settings
					else if((verMajor*100 + verMinor*10 + verPoint) > 
							(curMajor*100 + curMinor*10 + curPoint)) // some future version
						result = false; // reset settings
					else
						result = true;
				}
			}
						

			if(result)
			{
				// Global
				loadXMLSetting(WiiSettings.theme, "theme", sizeof(WiiSettings.theme));
				loadXMLSetting(&WiiSettings.language, "language");
				loadXMLSetting(&WiiSettings.volume, "volume");
				loadXMLSetting(&WiiSettings.hideExtensions, "hideExtensions");
				loadXMLSetting(&WiiSettings.exitAction, "exitAction");
				loadXMLSetting(&WiiSettings.rumble, "rumble");
				loadXMLSetting(&WiiSettings.screensaverDelay, "screensaverDelay");
				loadXMLSetting(&WiiSettings.inactivityShutdown, "inactivityShutdown");
				loadXMLSetting(&WiiSettings.lockFolders, "lockFolders");
				loadXMLSetting(&WiiSettings.startArea, "startArea");
				loadXMLSetting(&WiiSettings.debug, "debug");
				// Videos
				loadXMLSetting(&WiiSettings.videoZoomHor, "videoZoomHor");
				loadXMLSetting(&WiiSettings.videoZoomVert, "videoZoomVert");
				loadXMLSetting(&WiiSettings.videoXshift, "videoXshift");
				loadXMLSetting(&WiiSettings.videoYshift, "videoYshift");
				loadXMLSetting(&WiiSettings.frameDropping, "frameDropping");
				loadXMLSetting(&WiiSettings.aspectRatio, "aspectRatio");
				loadXMLSetting(&WiiSettings.cacheFill, "cacheFill");
				loadXMLSetting(WiiSettings.audioLanguage, "audioLanguage", sizeof(WiiSettings.audioLanguage));
				loadXMLSetting(&WiiSettings.audioDelay, "audioDelay");
				loadXMLSetting(&WiiSettings.autoResume, "autoResume");
				loadXMLSetting(&WiiSettings.autoPlayNextVideo, "autoPlayNextVideo");
				loadXMLSetting(&WiiSettings.skipBackward, "skipBackward");
				loadXMLSetting(&WiiSettings.skipForward, "skipForward");
				loadXMLSetting(WiiSettings.videosFolder, "videosFolder", sizeof(WiiSettings.videosFolder));
				// Music
				loadXMLSetting(&WiiSettings.playOrder, "playOrder");
				loadXMLSetting(WiiSettings.musicFolder, "musicFolder", sizeof(WiiSettings.musicFolder));
				// Pictures
				loadXMLSetting(&WiiSettings.slideshowDelay, "slideshowDelay");
				loadXMLSetting(WiiSettings.picturesFolder, "picturesFolder", sizeof(WiiSettings.picturesFolder));
				// DVD
				loadXMLSetting(&WiiSettings.dvdMenu, "dvdMenu");
				loadXMLSetting(&WiiSettings.dvdDisabled, "dvdDisabled");
				// Online Media
				loadXMLSetting(&WiiSettings.onlineCacheFill, "onlineCacheFill");
				loadXMLSetting(&WiiSettings.youtubeFormat, "youtubeFormat");
				loadXMLSetting(WiiSettings.onlinemediaFolder, "onlinemediaFolder", sizeof(WiiSettings.onlinemediaFolder));
				// Network
				
				for(int i=0; i<MAX_SHARES; i++)
				{
					loadXMLSMBShare(i);
					loadXMLFTPSite(i);
				}
				
				// Subtitles
				loadXMLSetting(&WiiSettings.subtitleVisibility, "subtitleVisibility");
				loadXMLSetting(&WiiSettings.subtitleDelay, "subtitleDelay");
				loadXMLSetting(WiiSettings.subtitleLanguage, "subtitleLanguage", sizeof(WiiSettings.subtitleLanguage));
				loadXMLSetting(WiiSettings.subtitleCodepage, "subtitleCodepage", sizeof(WiiSettings.subtitleCodepage));
				loadXMLSetting(WiiSettings.subtitleColor, "subtitleColor", sizeof(WiiSettings.subtitleColor));
				loadXMLSetting(&WiiSettings.subtitleSize, "subtitleSize");
			}
			mxmlDelete(xml);
		}
	}

	mem2_free(savebuffer, MEM2_OTHER);
	return result;
}

/****************************************************************************
 * Load settings
 * Checks sources consecutively until we find a settings file
 ***************************************************************************/
bool LoadSettings()
{
	static bool settingsLoaded = false;

	if(settingsLoaded) // already attempted loading
		return true;

	if(appPath[0] == 0)
		return false;

	bool settingsFound = false;
	char filepath[MAXPATHLEN];

	sprintf(filepath, "%s/settings.xml", appPath);
	settingsFound = LoadSettingsFile(filepath);
	sprintf(filepath, "%s/onlinemedia.xml", appPath);
	LoadOnlineMediaFile(filepath);

	settingsLoaded = true; // attempted to load settings
	if(settingsFound)
	{
		FixInvalidSettings();
		ChangeLanguage();

		if(WiiSettings.theme[0] != 0)
			ChangeTheme();

		sprintf(filepath,"%s/restore_points", appPath);
		char *buffer = (char *)mem2_malloc(50*1024, MEM2_OTHER);

		if(buffer)
		{
			int size = LoadFile(buffer, 50*1024, filepath, SILENT);
			if(size > 0)
				wiiLoadRestorePoints(buffer, size);
			mem2_free(buffer, MEM2_OTHER);
		}
	}
	return settingsFound;
}
