/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include <boost/algorithm/string.hpp>

#include "AppPaths.h"
#include "PathUtil.h"
#include "Util.h"
#include "LogManager.h"
#include "SimpleXML.h"
#include "AdcHub.h"
#include "UploadManager.h"
#include "ThrottleManager.h"
#include "ShareManager.h"
#include "SearchManager.h"
#include "FavoriteManager.h"
#include "ClientManager.h"
#include "SysVersion.h"
#include "Random.h"
#include "version.h"

#ifndef _WIN32
#define TRUE  1
#define FALSE 0

#define CW_USEDEFAULT ((int) 0x80000000)
#define SW_SHOWNORMAL 1
#define RGB(r, g, b) ((uint32_t)((r) & 0xFF) | (uint32_t)(((g) & 0xFF)<<8) | (uint32_t)(((b) & 0xFF)<<16))
#endif

static const string DEFAULT_LANG_FILE = "en-US.xml";

static const char URL_GET_IP_DEFAULT[]  = "http://checkip.dyndns.com";
static const char URL_GET_IP6_DEFAULT[] = "http://checkipv6.dynu.com";
static const char URL_GEOIP_DEFAULT[]   = "http://geoip.airdcpp.net";

static const char HUBLIST_SERVERS_DEFAULT[] =
	"https://www.te-home.net/?do=hublist&get=hublist.xml.bz2;"
	"https://dcnf.github.io/Hublist/hublist.xml.bz2;"
	"https://dchublist.biz/?do=hublist&get=hublist.xml.bz2;"
	"https://dchublist.org/hublist.xml.bz2;"
	"https://dchublist.ru/hublist.xml.bz2;"
	"https://dchublists.com/?do=hublist&get=hublist.xml.bz2";

static const char COMPRESSED_FILES_DEFAULT[] = "*.bz2;*.zip;*.rar;*.7z;*.gz;*.mp3;*.ogg;*.flac;*.ape;*.mp4;*.mkv;*.jpg;*.jpeg;*.gif;*.png;*.docx;*.xlsx";
static const char WANT_END_FILES_DEFAULT[] = "*.mov;*.mp4;*.3gp;*.3g2";

static const int MIN_SPEED_LIMIT = 32;

string SettingsManager::strSettings[STR_LAST - STR_FIRST];
int    SettingsManager::intSettings[INT_LAST - INT_FIRST];

string SettingsManager::strDefaults[STR_LAST - STR_FIRST];
int    SettingsManager::intDefaults[INT_LAST - INT_FIRST];

bool SettingsManager::isSet[SETTINGS_LAST];

static string getConfigFile()
{
	return Util::getConfigPath() + "DCPlusPlus.xml";
}

// Search types
SettingsManager::SearchTypes SettingsManager::g_searchTypes; // name, extlist

static const char* settingTags[] =
{
	// Strings //
	"ConfigVersion",
	
	// Language & encoding
	"LanguageFile",
	"DefaultCodepage",
	"TimeStampsFormat",

	// User settings
	"Nick",
	"CID",
	"UploadSpeed",
	"Description",
	"EMail",
	"ClientID",
	"DHTKey",
	
	// Network settings
	"BindAddress",
	"BindAddress6",
	"BindDevice",
	"BindDevice6",
	"ExternalIp",
	"ExternalIp6",
	"Mapper",
	"Mapper6",
	"SocksServer", "SocksUser", "SocksPassword",
	"HttpProxy",
	"HttpUserAgent",

	// Directories
	"DownloadDirectory",
	"TempDownloadDirectory",
	"DclstFolder",
	
	// Sharing
	"SkiplistShare",

	// Uploads
	"CompressedFiles",

	// Downloads & Queue
	"WantEndFiles",

	// Private messages
	"Password",
	"PasswordHint",
	"PasswordOkHint",

	// Auto priority
	"AutoPriorityPatterns",

	// URLs
	"HublistServers",
	"UrlPortTest",
	"UrlGetIp",
	"UrlGetIp6",
	"UrlDHTBootstrap",
	"UrlGeoIP",

	// TLS settings
	"TLSPrivateKeyFile", "TLSCertificateFile", "TLSTrustedCertificatesPath",

	// Protocol options
	"NMDCFeaturesCC",
	"ADCFeaturesCC",

	// Message templates
	"DefaultAwayMessage",
	"SecondaryAwayMsg",
	"SlotAskMessage",
	"RatioTemplate",
	"WebMagnetTemplate",
	
	// Web server
	"WebServerBindAddress",
	"WebServerUser", "WebServerPass", 
	"WebServerPowerUser", "WebServerPowerPass", 
	
	// Logging
	"LogDir",
	"LogFileDownload",
	"LogFileUpload",
	"LogFileMainChat",
	"LogFilePrivateChat",
	"LogFileStatus",
	"LogFileWebServer",
	"LogFileSystem",
	"LogFileSQLiteTrace",
	"LogFileTorrentTrace",
	"LogFileSearchTrace",
	"LogFileDHTTrace",
	"LogFilePSRTrace",
	"LogFileFloodTrace",
	"LogFileCMDDebugTrace",
	"LogFileUDPDebugTrace",
	"LogFileTLSCert",
	"LogFormatPostDownload",
	"LogFormatPostUpload",
	"LogFormatMainChat",
	"LogFormatPrivateChat",
	"LogFormatStatus", 
	"LogFormatWebServer",
	"LogFormatSystem",	
	"LogFormatSQLiteTrace",
	"LogFormatTorrentTrace",
	"LogFormatSearchTrace",
	"LogFormatDHTTrace",
	"LogFormatPSRTrace",
	"LogFormatFloodTrace",
	"LogFormatCMDDebugTrace",
	"LogFormatUDPDebugTrace",

	// Players formats
	"WinampFormat", "WMPFormat", "iTunesFormat", "MPCFormat", "JetAudioFormat", "QcdQmpFormat",

	// Font
	"TextFont",

	// Toolbar settings
	"Toolbar",
	"WinampToolBar",

	// Popup settings
	"PopupFont",
	"PopupTitleFont",
	"PopupImageFile",
	
	// Sounds
	"SoundBeepFile", "SoundBeginFile", "SoundFinishedFile", "SoundSourceFile",
	"SoundUploadFile", "SoundFakerFile", "SoundChatNameFile", "SoundTTH",
	"SoundHubConnected", "SoundHubDisconnected", "SoundFavUserOnline", "SoundFavUserOffline",
	"SoundTypingNotify", "SoundSearchSpy",

	// Themes and custom images
	"ColorTheme",
	"UserListImage",
	"ThemeDLLName",
	"ThemeManagerSoundsThemeName",
	"EmoticonsFile",
	"AdditionalEmoticons",

	// Password
	"AuthPass",

	// Frames UI state
	"TransferFrameOrder", "TransferFrameWidths", "TransferFrameVisible",
	"HubFrameOrder", "HubFrameWidths", "HubFrameVisible",
	"SearchFrameOrder", "SearchFrameWidths", "SearchFrameVisible",
	"DirectoryListingFrameOrder", "DirectoryListingFrameWidths", "DirectoryListingFrameVisible",
	"FavoritesFrameOrder", "FavoritesFrameWidths", "FavoritesFrameVisible",	
	"QueueFrameOrder", "QueueFrameWidths", "QueueFrameVisible",
	"PublicHubsFrameOrder", "PublicHubsFrameWidths", "PublicHubsFrameVisible",
	"UsersFrameOrder", "UsersFrameWidths", "UsersFrameVisible", 
	"FinishedDLFrameOrder", "FinishedDLFrameWidths", "FinishedDLFrameVisible", 
	"FinishedULFrameWidths", "FinishedULFrameOrder", "FinishedULFrameVisible",
	"UploadQueueFrameOrder", "UploadQueueFrameWidths", "UploadQueueFrameVisible",
	"RecentFrameOrder", "RecentFrameWidths", "RecentFrameVisible",
	"ADLSearchFrameOrder", "ADLSearchFrameWidths", "ADLSearchFrameVisible",
	"SpyFrameWidths", "SpyFrameOrder", "SpyFrameVisible",	

	// Recents
	"SavedSearchSize",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05",
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10",
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15",
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",

	"SENTRY",
	
	// Ints //

	// User settings
	"Gender",
	"OverrideClientID",
	"ExtDescription",
	"ExtDescriptionSlots",
	"ExtDescriptionLimit",
	"AutoChangeNick",
	
	// Network settings (Ints)
	"EnableIP6",
	"InPort",
	"UDPPort",
	"TLSPort",
	"UseTLS", 
	"DHTPort",
	"IncomingConnections",
	"IncomingConnections6",
	"OutgoingConnections",
	"AutoDetectIncomingConnection",
	"AutoDetectIncomingConnection6",
	"AllowNATTraversal",
	"AutoUpdateIP",
	"WANIPManual",
	"WANIPManual6",
	"BindOptions",
	"BindOptions6",
	"AutoUpdateIPInterval",
	"NoIPOverride",
	"NoIPOverride6",
	"AutoTestPorts",
	"SocksPort", "SocksResolve",
	"UseDHT",
	"UseHTTPProxy",

	// Slots & policy
	"Slots",
	"AutoKick",
	"AutoKickNoFavs",
	"MinislotSize",
	"ExtraSlots",
	"HubSlots",
	"ExtraSlotByIP",
	"ExtraSlotToDl",
	"ExtraPartialSlots",
	"AutoSlot",
	"AutoSlotMinULSpeed",
	"SendSlotGrantMsg",
	
	// Protocol options (Ints)
	"SocketInBuffer2",
	"SocketOutBuffer2",
	"CompressTransfers",
	"MaxCompression",
	"SendBloom",
	"SendExtJSON",
	"SendDBParam",
	"SendQPParam",
	"UseSaltPass",
	"UseBotList",
	"UseMCTo",
	"UseCCPM",
	"UseCPMI",
	"CCPMAutoStart",
	"CCPMIdleTimeout",
	"UseTL",
	"UseDIParam",
	"MaxCommandLength",
	"HubUserCommands",
	"MaxHubUserCommands",
	"MyInfoDelay",
	"NMDCEncodingFromDomain",

	// Sharing
	"AutoRefreshTime",
	"AutoRefreshOnStartup",
	"ShareHidden",
	"ShareSystem",
	"ShareVirtual",
	"MaxHashSpeed",
	"SaveTthInNtfsFilestream",
	"SetMinLengthTthInNtfsFilestream",
	"FastHash",
	"MediaInfoOptions",
	"MediaInfoForceUpdate",

	// File lists
	"FileListUseUploadCount",
	"FileListUseTS",
	"FileListShowShared",
	"FileListShowDownloaded",
	"FileListShowCanceled",
	"FileListShowMyUploads",

	// Downloads & Queue
	"DownloadSlots",
	"FileSlots",
	"ExtraDownloadSlots",
	"MaxDownloadSpeed",
	"BufferSizeForDownloads",
	"EnableMultiChunk",
	"MinMultiChunkSize",
	"MaxChunkSize",
	"OverlapChunks",
	"DownConnPerSec",
	"AutoSearch",
	"AutoSearchTime",
	"AutoSearchLimit",
	"AutoSearchAutoMatch",
	"AutoSearchMaxSources",
	"ReportFoundAlternates",
	"DontBeginSegment",
	"DontBeginSegmentSpeed", 
	"SegmentsManual",
	"NumberOfSegments",
	"SkipZeroByte",
	"DontDownloadAlreadyShared",
	"KeepListsDays",
	"TargetExistsAction",
	"SkipExisting",
	"CopyExistingMaxSize",
	"UseMemoryMappedFiles",
	"SearchMagnetSources",
	"AutoMatchDownloadedLists",

	// Slow sources auto disconnect
	"AutoDisconnectEnable",
	"AutoDisconnectSpeed",
	"AutoDisconnectFileSpeed",
	"AutoDisconnectTime",
	"AutoDisconnectMinFileSize",
	"AutoDisconnectRemoveSpeed",
	"DropMultiSourceOnly",

	// Private messages (Ints)
	"ProtectPrivate",
	"ProtectPrivateRnd",
	"ProtectPrivateSay",
	"PMLogLines",
	"IgnoreMe",
	"SuppressPms",
	"LogIfSuppressPms",
	"IgnoreHubPms",
	"IgnoreBotPms",
	
	// Max finished items
	"MaxFinishedDownloads",
	"MaxFinishedUploads",

	// Throttling
	"ThrottleEnable",
	"UploadLimitNormal",
	"UploadLimitTime",
	"DownloadLimitNormal",
	"DownloadLimitTime",
	"TimeThrottle",
	"TimeLimitStart",
	"TimeLimitEnd",
	"PerUserUploadLimit",

	// User checking (Ints)
	"CheckUsersNmdc",
	"CheckUsersAdc",
	"UserCheckBatch",

	// Auto priority (Ints)
	"AutoPriorityUsePatterns",
	"AutoPriorityPatternsPrio",
	"AutoPriorityUseSize",
	"AutoPrioritySmallSize",
	"AutoPrioritySmallSizePrio",

	// Malicious IP detection
	"EnableIpGuard",
	"EnableP2PGuard",
	"EnableIpTrust",
	"IpGuardDefaultDeny",
	"P2PGuardLoadIni",
	"P2PGuardBlock",

	// Anti-flood
	"AntiFloodMinReqCount",
	"AntiFloodMaxReqPerMin",
	"AntiFloodBanTime",

	// Search
	"SearchPassiveAlways", 
	"MinimumSearchInterval",
	"MinimumSearchIntervalPassive",
	"IncomingSearchTTHOnly",
	"IncomingSearchIgnoreBots",
	"IncomingSearchIgnorePassive",

	// Away settings
	"Away",
	"AutoAway",
	"AwayThrottle",
	"AwayStart",
	"AwayEnd",
	
	// TLS settings (Ints)
	"AllowUntrustedHubs",
	"AllowUntrustedClients",

	// Database
	"DbLogFinishedDownloads",
	"DbLogFinishedUploads",
	"EnableLastIP",
	"EnableRatioUserList",
	"EnableUploadCounter",
	"SQLiteJournalMode",
	"DbFinishedBatch",
	"GeoIPAutoUpdate",
	"GeoIPCheckHours",
	"UseCustomLocations",

	// Web server (Ints)
	"WebServer",
	"WebServerPort",

	// Logging (Ints)
	"LogDownloads",
	"LogUploads",
	"LogMainChat",
	"LogPrivateChat",
	"LogStatusMessages", 
	"WebServerLog",
	"LogSystem",
	"LogSQLiteTrace",
	"LogTorrentTrace",
	"LogSearchTrace",
	"LogDHTTrace",
	"LogPSRTrace",
	"LogFloodTrace",
	"LogFilelistTransfers",
	"LogCMDDebugTrace",
	"LogUDPDebugTrace",
	"LogSocketInfo",
	"LogTLSCertificates",

	// Startup & shutdown
	"StartupBackup",
	"ShutdownAction",
	"ShutdownTimeout",
	"UrlHandler",
	"MagnetRegister",
	"DclstRegister",
	"DetectPreviewApps",

	// Theme
	"ColorThemeModified",

	// Colors & text styles
	"BackgroundColor",
	"TextColor",
	"ErrorColor",
	"TextGeneralBackColor", "TextGeneralForeColor", "TextGeneralBold", "TextGeneralItalic",
	"TextMyOwnBackColor", "TextMyOwnForeColor", "TextMyOwnBold", "TextMyOwnItalic",
	"TextPrivateBackColor", "TextPrivateForeColor", "TextPrivateBold", "TextPrivateItalic",
	"TextSystemBackColor", "TextSystemForeColor", "TextSystemBold", "TextSystemItalic",
	"TextServerBackColor", "TextServerForeColor", "TextServerBold", "TextServerItalic",
	"TextTimestampBackColor", "TextTimestampForeColor", "TextTimestampBold", "TextTimestampItalic",
	"TextMyNickBackColor", "TextMyNickForeColor", "TextMyNickBold", "TextMyNickItalic",
	"TextNormBackColor", "TextNormForeColor", "TextNormBold", "TextNormItalic",
	"TextFavBackColor", "TextFavForeColor", "TextFavBold", "TextFavItalic",
	"TextOPBackColor", "TextOPForeColor", "TextOPBold", "TextOPItalic",
	"TextURLBackColor", "TextURLForeColor", "TextURLBold", "TextURLItalic",
	"TextEnemyBackColor", "TextEnemyForeColor", "TextEnemyBold", "TextEnemyItalic",

	// User list colors
	"ReservedSlotColor",
	"IgnoredColor",
	"FavoriteColor",
	"FavBannedColor",
	"NormalColor",
	"FireballColor",
	"ServerColor",
	"PassiveColor",
	"OpColor",
	"CheckedColor",
	"CheckedFailColor",

	// Other colors
	"DownloadBarColor",
	"UploadBarColor",
	"ProgressBackColor",
	"ProgressSegmentColor",	
	"ColorRunning",
	"ColorRunning2",
	"ColorDownloaded",
	"BanColor",
	"FileSharedColor",
	"FileDownloadedColor",
	"FileCanceledColor",
	"FileFoundColor",
	"FileQueuedColor",
	"TabsInactiveBgColor",
	"TabsActiveBgColor",
	"TabsInactiveTextColor",
	"TabsActiveTextColor",
	"TabsOfflineBgColor",
	"TabsOfflineActiveBgColor",
	"TabsUpdatedBgColor",
	"TabsBorderColor",

	// Assorted UI settings (Ints)
	"AppDpiAware",
	"ShowGrid",
	"ShowInfoTips",
	"UseSystemIcons", 
	"MDIMaxmimized",
	"ToggleActiveWindow",
	"PopunderPm",
	"PopunderFilelist",
	
	// Tab settings
	"TabsPos", "MaxTabRows", "TabSize", "TabsShowInfoTips",
	"UseTabsCloseButton", "TabsBold", "NonHubsFront",
	"BoldFinishedDownloads", "BoldFinishedUploads", "BoldQueue",
	"BoldHub", "BoldPm", "BoldSearch", "BoldWaitingUsers",
	"HubUrlInTitle",

	// Toolbar settings (Ints)
	"LockToolbars",
	"TbImageSize",
	"ShowWinampControl", 
	
	// Menu settings
	"UseCustomMenu", "MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped",
	"UcSubMenu",
	
	// Progressbar settings
	"ShowProgressBars",
	"ProgressTextDown", "ProgressTextUp",
	"ProgressOverrideColors", "Progress3DDepth", "ProgressOverrideColors2",
	"ProgressbaroDCStyle", "OdcStyleBumped",
	"StealthyStyleIco", "StealthyStyleIcoSpeedIgnore", 
	"TopDownSpeed",
	"TopUpSpeed",

	// Popup settings (Ints)
	"PopupsDisabled",
	"PopupType",
	"PopupTime",
	"PopupW",
	"PopupH",
	"PopupTransp",
	"PopupMaxLength",
	"PopupBackColor",
	"PopupTextColor",
	"PopupTitleTextColor",
	"PopupImage",
	"PopupColors", 
	"PopupHubConnected", "PopupHubDisconnected",
	"PopupFavoriteConnected", "PopupFavoriteDisconnected",
	"PopupCheatingUser", "PopupChatLine",
	"PopupDownloadStart", "PopupDownloadFailed", "PopupDownloadFinished",
	"PopupUploadFinished",
	"PopupPm", "PopupNewPM",
	"PopupSearchSpy",
	"PopupNewFolderShare",
	"PreviewPm",
	"PopupAway",
	"PopupMinimized", 

	// Sound settings (Ints)
	"SoundsDisabled",
	"PrivateMessageBeep",
	"PrivateMessageBeepOpen",
	
	// Open on startup
	"OpenRecentHubs", "OpenPublic", "OpenFavoriteHubs", "OpenFavoriteUsers",
	"OpenQueue", "OpenFinishedDownloads", "OpenFinishedUploads",
	"OpenSearchSpy", "OpenNetworkStatistics", "OpenNotepad",
	"OpenWaitingUsers", "OpenCdmDebug", "OpenDHT", "OpenADLSearch",

	// Click actions
	"UserListDoubleClick",
	"TransferListDoubleClick",
	"ChatDoubleClick", 
	"FavUserDblClick",
	"MagnetAsk",
	"MagnetAction",
	"SharedMagnetAsk",
	"SharedMagnetAction",
	"DCLSTAsk",
	"DCLSTAction",

	// Window behavior
	"Topmost",
	"MinimizeToTray",
	"MinimizeOnStartup",
	"MinimizeOnClose",
	"ShowCurrentSpeedInTitle",

	// Confirmations
	"ConfirmExit",
	"ConfirmDelete",
	"ConfirmHubRemoval",
	"ConfirmHubgroupRemoval",
	"ConfirmUserRemoval", 
	"ConfirmFinishedRemoval",
	"ConfirmShareFromShell",
	"ConfirmClearSearchHistory",
	"ConfirmAdlsRemoval",

	// Password (Ints)
	"ProtectTray", "ProtectStart", "ProtectClose",
	
	// ADLSearch
	"AdlsBreakOnFirst",

	// Media player
	"MediaPlayer",
	"UseMagnetsInPlayerSpam",
	"UseBitrateFixForSpam",
	
	// Other settings, mostly useless
	"ReduceProcessPriorityIfMinimized",
	"DclstCreateInSameFolder",
	"DclstIncludeSelf",
	
	// View visibility
	"ShowStatusbar", "ShowToolbar", "ShowTransferView", "ShowTransferViewToolbar", "ShowQSearch",
	
	// Frames UI state (Ints)
	"TransferFrameSort", "TransferFrameSplit",
	"HubFrameSort",
	"SearchFrameSort",
	"DirectoryListingFrameSort", "DirectoryListingFrameSplit",
	"FavoritesFrameSort",
	"QueueFrameSort", "QueueFrameSplit",
	"PublicHubsFrameSort",
	"UsersFrameSort",
	"UsersFrameSplit",
	"FinishedDLFrameSort",
	"FinishedULFrameSort",
	"UploadQueueFrameSort", "UploadQueueFrameSplit",
	"RecentFrameSort",
	"ADLSearchFrameSort",
	"SpyFrameSort",

	// Hub frame
	"AutoFollow",
	"ShowJoins",
	"FavShowJoins",
	"PromptPassword",
	"FilterMessages",
	"EnableCountryFlag",
	"ShowCheckedUsers",
	"HubPosition",
	"SortFavUsersFirst",
	"FilterEnter",
	"OpenNewWindow",
	"HubThreshold",
	"PopupHubPms",
	"PopupBotPms", 
	"PopupPMs",
	
	// Chat frame
	"ChatBufferSize",
	"ChatTimeStamps",
	"BoldMsgAuthor",
	"ChatShowInfoTips",
	"ShowEmoticonsButton",
	"ChatAnimSmiles",
	"SmileSelectWndAnimSmiles",
	"ShowSendMessageButton",
#ifdef BL_UI_FEATURE_BB_CODES
	"FormatBBCode",
	"FormatBBCodeColors",
	"ShowBBCodePanel",
#endif
	"ShowMultiChatButton",
	"ShowTranscodeButton",
	"ShowLinkButton",
	"ShowFindButton",
	"ChatRefferingToNick",
	"IpInChat",
	"CountryInChat",
	"ISPInChat", 
	"StatusInChat",
	"DisplayCheatsInMainChat",
	"UseCTRLForLineHistory",
	"MultilineChatInput",
	"MultilineChatInputCtrlEnter",
#ifdef BL_UI_FEATURE_BB_CODES
	"FormatBotMessage",
#endif
	"SendUnknownCommands",
	"SuppressMainChat",

	// Search frame
	"SaveSearchSettings",
	"ForgetSearchRequest", 
	"SearchHistory",
	"ClearSearch",
	"UseSearchGroupTreeSettings",
	"OnlyFreeSlots",

	// Users frame
	"ShowIgnoredUsers",

	// Upload queue frame
	"UploadQueueFrameShowTree",

	// Finished frame
	"ShowShellMenu",
	
	// Search Spy frame
	"SpyFrameIgnoreTthSearches",
	"ShowSeekersInSpyFrame",
	"LogSeekersInSpyFrame",
	
	// Transfers
	"TransfersOnlyActiveUploads",

	// Settings dialog
	"RememberSettingsPage",
	"SettingsPage",
	"UseOldSharingUI",

	// Main window size & position
	"MainWindowState",
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY",

	// Recents
	"SavedSearchType", "SavedSearchSizeMode", "SavedSearchMode",
	
	"SENTRY",
};

SettingsManager::SettingsManager()
{
	BOOST_STATIC_ASSERT(_countof(settingTags) == SETTINGS_LAST + 1);
	
	for (size_t i = 0; i < SETTINGS_LAST; i++)
		isSet[i] = false;
		
	memset(intDefaults, 0, sizeof(intDefaults));
	memset(intSettings, 0, sizeof(intSettings));
}

void SettingsManager::setDefaults()
{
#ifdef _DEBUG
	static bool settingsInitialized = false;
	dcassert(!settingsInitialized);
	if (settingsInitialized) return;
	settingsInitialized = true;
#endif

	// Language & encoding
	setDefault(LANGUAGE_FILE, DEFAULT_LANG_FILE);
	setDefault(TIME_STAMPS_FORMAT, "%X");

	// User settings
	setDefault(UPLOAD_SPEED, "50"); // 50 Mbit/s

	// Network settings
	setDefault(BIND_ADDRESS, "0.0.0.0");

	// Directories
	setDefault(DOWNLOAD_DIRECTORY, Util::getDownloadsPath());
	
	// Sharing
	setDefault(SKIPLIST_SHARE, "*.dctmp;*.!ut");

	// Uploads
	setDefault(COMPRESSED_FILES, COMPRESSED_FILES_DEFAULT);

	// Downloads & Queue
	setDefault(WANT_END_FILES, WANT_END_FILES_DEFAULT);

	// Private messages
	setDefault(PM_PASSWORD, Util::getRandomNick()); // Generate a random PM password
	setDefault(PM_PASSWORD_HINT, STRING(DEF_PASSWORD_HINT));
	setDefault(PM_PASSWORD_OK_HINT, STRING(DEF_PASSWORD_OK_HINT));

	// Auto priority
	setDefault(AUTO_PRIORITY_PATTERNS, "*.sfv;*.nfo;*sample*;*cover*;*.pls;*.m3u");
	
	// URLs
	setDefault(HUBLIST_SERVERS, HUBLIST_SERVERS_DEFAULT);
	setDefault(URL_PORT_TEST, "http://media.fly-server.ru:37015/fly-test-port");
	setDefault(URL_GET_IP, URL_GET_IP_DEFAULT);
	setDefault(URL_GET_IP6, URL_GET_IP6_DEFAULT);
	setDefault(URL_DHT_BOOTSTRAP, "http://strongdc.sourceforge.net/bootstrap/");
	setDefault(URL_GEOIP, URL_GEOIP_DEFAULT);

	// TLS settings
	const string tlsPath = Util::getConfigPath() + "Certificates" PATH_SEPARATOR_STR;
	setDefault(TLS_PRIVATE_KEY_FILE, tlsPath + "client.key");
	setDefault(TLS_CERTIFICATE_FILE, tlsPath + "client.crt");
	setDefault(TLS_TRUSTED_CERTIFICATES_PATH, tlsPath);

	// Message templates
	setDefault(DEFAULT_AWAY_MESSAGE, STRING(DEFAULT_AWAY_MESSAGE));
	setDefault(ASK_SLOT_MESSAGE, STRING(ASK_SLOT_TEMPLATE));
	setDefault(RATIO_MESSAGE, "/me ratio: %[ratio] (Uploaded: %[up] | Downloaded: %[down])");
	setDefault(WMLINK_TEMPLATE, "<a href=\"%[magnet]\" title=\"%[name]\" target=\"_blank\">%[name] (%[size])</a>");

	// Web server
	setDefault(WEBSERVER_BIND_ADDRESS, "0.0.0.0");
	setDefault(WEBSERVER_USER, "user");
	setDefault(WEBSERVER_POWER_USER, "admin");

	// Logging
	setDefault(LOG_DIRECTORY, Util::getLocalPath() + "Logs" PATH_SEPARATOR_STR);
	setDefault(LOG_FILE_DOWNLOAD, "Downloads.log");
	setDefault(LOG_FILE_UPLOAD, "Uploads.log");
	setDefault(LOG_FILE_MAIN_CHAT, "%Y-%m" PATH_SEPARATOR_STR "%[hubURL].log");
	setDefault(LOG_FILE_PRIVATE_CHAT, "PM" PATH_SEPARATOR_STR "%Y-%m" PATH_SEPARATOR_STR "%[userNI]-%[hubURL].log");
	setDefault(LOG_FILE_STATUS, "%Y-%m" PATH_SEPARATOR_STR  "%[hubURL]_status.log");
	setDefault(LOG_FILE_WEBSERVER, "WebServer.log");
	setDefault(LOG_FILE_SYSTEM, "System.log");
	setDefault(LOG_FILE_SQLITE_TRACE, "SQLTrace.log");
	setDefault(LOG_FILE_TORRENT_TRACE, "torrent.log");
	setDefault(LOG_FILE_SEARCH_TRACE, "Found.log");
	setDefault(LOG_FILE_DHT_TRACE, "DHT.log");
	setDefault(LOG_FILE_PSR_TRACE, "PSR.log");
	setDefault(LOG_FILE_FLOOD_TRACE, "flood.log");
	setDefault(LOG_FILE_TCP_MESSAGES, "Trace" PATH_SEPARATOR_STR "%[ip]" PATH_SEPARATOR_STR "%[ipPort].log");
	setDefault(LOG_FILE_UDP_PACKETS, "Trace" PATH_SEPARATOR_STR "UDP-Packets.log");
	setDefault(LOG_FILE_TLS_CERT, "Trace" PATH_SEPARATOR_STR "%[ip]" PATH_SEPARATOR_STR "%[kp].cer");
	setDefault(LOG_FORMAT_DOWNLOAD, "%Y-%m-%d %H:%M:%S: %[target] " + STRING(DOWNLOADED_FROM) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_UPLOAD, "%Y-%m-%d %H:%M:%S %[source] " + STRING(UPLOADED_TO) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M:%S%[extra]] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M:%S%[extra]] %[message]");
	setDefault(LOG_FORMAT_STATUS, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_WEBSERVER, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_SYSTEM, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_SQLITE_TRACE, "[%Y-%m-%d %H:%M:%S] (%[thread_id]) %[sql]");
	setDefault(LOG_FORMAT_TORRENT_TRACE, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_SEARCH_TRACE, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_DHT_TRACE, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_PSR_TRACE, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_FLOOD_TRACE, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_TCP_MESSAGES, "[%Y-%m-%d %H:%M:%S] %[message]");
	setDefault(LOG_FORMAT_UDP_PACKETS, "[%Y-%m-%d %H:%M:%S] %[message]");

	// Players formats
	setDefault(WINAMP_FORMAT, "+me is listening to '%[artist] - %[track] - %[title]' (%[percent] of %[length], %[bitrate], Winamp %[version]) %[magnet]");
	setDefault(WMP_FORMAT, "+me is listening to '%[title]' (%[bitrate], Windows Media Player %[version]) %[magnet]");
	setDefault(ITUNES_FORMAT, "+me is listening to '%[artist] - %[title]' (%[percent] of %[length], %[bitrate], iTunes %[version]) %[magnet]");
	setDefault(MPLAYERC_FORMAT, "+me is listening to '%[title]' (Media Player Classic) %[magnet]");
	setDefault(JETAUDIO_FORMAT, "+me is listening to '%[artist] - %[title]' (%[percent], JetAudio %[version]) %[magnet]");
	setDefault(QCDQMP_FORMAT, "+me is listening to '%[title]' (%[bitrate], %[sample]) (%[elapsed] %[bar] %[length]) %[magnet]");

	// Toolbar settings
	setDefault(TOOLBAR, "1,27,-1,0,25,5,3,4,-1,6,7,8,9,22,-1,10,-1,14,23,-1,15,17,-1,19,21,29,24,28,20");
	setDefault(WINAMPTOOLBAR, "0,-1,1,2,3,4,5,6,7,8");

	// Sounds
	setDefault(SOUND_BEEPFILE, Util::getSoundsPath() + "PrivateMessage.wav");
	setDefault(SOUND_BEGINFILE, Util::getSoundsPath() + "DownloadBegins.wav");
	setDefault(SOUND_FINISHFILE, Util::getSoundsPath() + "DownloadFinished.wav");
	setDefault(SOUND_SOURCEFILE, Util::getSoundsPath() + "AltSourceAdded.wav");
	setDefault(SOUND_UPLOADFILE, Util::getSoundsPath() + "UploadFinished.wav");
	setDefault(SOUND_FAKERFILE, Util::getSoundsPath() + "FakerFound.wav");
	setDefault(SOUND_CHATNAMEFILE, Util::getSoundsPath() + "MyNickInMainChat.wav");
	setDefault(SOUND_TTH, Util::getSoundsPath() + "FileCorrupted.wav");
	setDefault(SOUND_HUBCON, Util::getSoundsPath() + "HubConnected.wav");
	setDefault(SOUND_HUBDISCON, Util::getSoundsPath() + "HubDisconnected.wav");
	setDefault(SOUND_FAVUSER, Util::getSoundsPath() + "FavUser.wav");
	setDefault(SOUND_FAVUSER_OFFLINE, Util::getSoundsPath() + "FavUserDisconnected.wav");
	setDefault(SOUND_TYPING_NOTIFY, Util::getSoundsPath() + "TypingNotify.wav");
	setDefault(SOUND_SEARCHSPY, Util::getSoundsPath() + "SearchSpy.wav");

	// Themes and custom images
	setDefault(EMOTICONS_FILE, "Kolobok");
	setDefault(ADDITIONAL_EMOTICONS, "FlylinkSmilesInternational;FlylinkSmiles");

	// Frames UI state
	setDefault(HUB_FRAME_VISIBLE, "1,1,0,1,1,1,1,1,1,1,1,1,1,1");
	setDefault(DIRLIST_FRAME_VISIBLE, "1,1,1,1,1");
	setDefault(FINISHED_DL_FRAME_VISIBLE, "1,1,1,1,1,1,1,1");
	setDefault(FINISHED_UL_FRAME_VISIBLE, "1,1,1,1,1,1,1");

	// Network settings (Ints)
	setDefault(TCP_PORT, 0);
	setDefault(UDP_PORT, 0);
	setDefault(TLS_PORT, 0);
	setDefault(USE_TLS, TRUE);
	setDefault(DHT_PORT, 0);
	setDefault(INCOMING_CONNECTIONS, INCOMING_FIREWALL_UPNP); // [!] IRainman default passive -> incoming firewall upnp
	setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
	setDefault(AUTO_DETECT_CONNECTION, TRUE);
	setDefault(AUTO_DETECT_CONNECTION6, TRUE);
	setDefault(ALLOW_NAT_TRAVERSAL, TRUE);	
	setDefault(AUTO_TEST_PORTS, TRUE);
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, TRUE);		

	// Slots & policy
	setDefault(SLOTS, 15);
	setDefault(MINISLOT_SIZE, 64);
	setDefault(EXTRA_SLOTS, 5);
	setDefault(EXTRA_SLOT_TO_DL, TRUE);
	setDefault(EXTRA_PARTIAL_SLOTS, 1);
	setDefault(AUTO_SLOTS, 5);

	// Protocol options
#ifdef OSVER_WIN_XP
	setDefault(SOCKET_IN_BUFFER, MAX_SOCKET_BUFFER_SIZE);
	setDefault(SOCKET_OUT_BUFFER, MAX_SOCKET_BUFFER_SIZE);
	/*
	  // Increase the socket buffer sizes from the default sizes for WinXP.  In
	  // performance testing, there is substantial benefit by increasing from 8KB
	  // to 64KB.
	  // See also:
	  //    http://support.microsoft.com/kb/823764/EN-US
	  // On Vista, if we manually set these sizes, Vista turns off its receive
	  // window auto-tuning feature.
	  //    http://blogs.msdn.com/wndp/archive/2006/05/05/Winhec-blog-tcpip-2.aspx
	  // Since Vista's auto-tune is better than any static value we can could set,
	  // only change these on pre-vista machines.
	*/
#endif // OSVER_WIN_XP
	setDefault(COMPRESS_TRANSFERS, TRUE);
	setDefault(MAX_COMPRESSION, 9);
	setDefault(SEND_BLOOM, TRUE);
	setDefault(SEND_EXT_JSON, TRUE);
	setDefault(SEND_DB_PARAM, TRUE);
	setDefault(SEND_QP_PARAM, TRUE);
	setDefault(USE_SALT_PASS, TRUE);
	setDefault(USE_BOT_LIST, TRUE);
	setDefault(USE_CCPM, TRUE);
	setDefault(USE_CPMI, TRUE);
	setDefault(CCPM_IDLE_TIMEOUT, 10);
	setDefault(USE_TTH_LIST, TRUE);
	setDefault(MAX_COMMAND_LENGTH, 16 * 1024 * 1024);
	setDefault(HUB_USER_COMMANDS, TRUE);
	setDefault(MAX_HUB_USER_COMMANDS, 500);
	setDefault(MYINFO_DELAY, 35);
	setDefault(NMDC_ENCODING_FROM_DOMAIN, TRUE);

	// Sharing (Ints)
	setDefault(AUTO_REFRESH_TIME, 60);
	setDefault(AUTO_REFRESH_ON_STARTUP, TRUE);
	setDefault(SHARE_VIRTUAL, TRUE);
	setDefault(SAVE_TTH_IN_NTFS_FILESTREAM, TRUE);
	setDefault(SET_MIN_LENGTH_TTH_IN_NTFS_FILESTREAM, 16);
	setDefault(FAST_HASH, TRUE);

	// File lists
	setDefault(FILELIST_INCLUDE_TIMESTAMP, TRUE);
	setDefault(FILELIST_SHOW_SHARED, TRUE);
	setDefault(FILELIST_SHOW_DOWNLOADED, TRUE);
	setDefault(FILELIST_SHOW_CANCELED, TRUE);
	setDefault(FILELIST_SHOW_MY_UPLOADS, TRUE);

	// Downloads & Queue
	setDefault(EXTRA_DOWNLOAD_SLOTS, 3);
	setDefault(BUFFER_SIZE_FOR_DOWNLOADS, 1024);
	setDefault(ENABLE_MULTI_CHUNK, TRUE);
	setDefault(MIN_MULTI_CHUNK_SIZE, 2);
	setDefault(OVERLAP_CHUNKS, TRUE);
	setDefault(DOWNCONN_PER_SEC, 2);
	setDefault(AUTO_SEARCH, TRUE);	
	setDefault(AUTO_SEARCH_TIME, 1);
	setDefault(AUTO_SEARCH_LIMIT, 15);
	setDefault(AUTO_SEARCH_MAX_SOURCES, 5);
	setDefault(REPORT_ALTERNATES, TRUE);
	setDefault(DONT_BEGIN_SEGMENT_SPEED, 1024);
	setDefault(SEGMENTS_MANUAL, TRUE);
	setDefault(NUMBER_OF_SEGMENTS, 50);
	setDefault(KEEP_LISTS_DAYS, 30);
	setDefault(TARGET_EXISTS_ACTION, TE_ACTION_ASK);
	setDefault(SKIP_EXISTING, TRUE);
	setDefault(COPY_EXISTING_MAX_SIZE, 100);
	setDefault(USE_MEMORY_MAPPED_FILES, TRUE);
	setDefault(SEARCH_MAGNET_SOURCES, TRUE);
	setDefault(AUTO_MATCH_DOWNLOADED_LISTS, TRUE);

	// Slow sources auto disconnect
	setDefault(AUTO_DISCONNECT_SPEED, 5);
	setDefault(AUTO_DISCONNECT_FILE_SPEED, 10);
	setDefault(AUTO_DISCONNECT_TIME, 20);
	setDefault(AUTO_DISCONNECT_MIN_FILE_SIZE, 10);
	setDefault(AUTO_DISCONNECT_REMOVE_SPEED, 2);
	setDefault(AUTO_DISCONNECT_MULTISOURCE_ONLY, TRUE);

	// Private messages (Ints)
	setDefault(PROTECT_PRIVATE_RND, 1);
	setDefault(PM_LOG_LINES, 50);
	setDefault(LOG_IF_SUPPRESS_PMS, TRUE);

	// Max finished items
	setDefault(MAX_FINISHED_DOWNLOADS, 1000);
	setDefault(MAX_FINISHED_UPLOADS, 1000);

	// User checking
	setDefault(USER_CHECK_BATCH, 5);

	// Auto priority (Ints)
	setDefault(AUTO_PRIORITY_USE_PATTERNS, TRUE);
	setDefault(AUTO_PRIORITY_PATTERNS_PRIO, QueueItem::HIGHER);
	setDefault(AUTO_PRIORITY_USE_SIZE, TRUE);
	setDefault(AUTO_PRIORITY_SMALL_SIZE, 64);
	setDefault(AUTO_PRIORITY_SMALL_SIZE_PRIO, QueueItem::HIGHER);

	// Malicious IP detection
	setDefault(ENABLE_IPTRUST, TRUE);

	// Anti-flood
	setDefault(ANTIFLOOD_MIN_REQ_COUNT, 15);
	setDefault(ANTIFLOOD_MAX_REQ_PER_MIN, 12);
	setDefault(ANTIFLOOD_BAN_TIME, 3600);

	// Search
	setDefault(MIN_SEARCH_INTERVAL, 10);
	setDefault(MIN_SEARCH_INTERVAL_PASSIVE, 10);

	// TLS settings (Ints)
	setDefault(ALLOW_UNTRUSTED_HUBS, TRUE);
	setDefault(ALLOW_UNTRUSTED_CLIENTS, TRUE);

	// Database
	setDefault(DB_LOG_FINISHED_DOWNLOADS, 365);	
	setDefault(DB_LOG_FINISHED_UPLOADS, 365);
	setDefault(ENABLE_LAST_IP_AND_MESSAGE_COUNTER, TRUE);
	setDefault(ENABLE_RATIO_USER_LIST, TRUE);
	setDefault(ENABLE_UPLOAD_COUNTER, TRUE);
	setDefault(DB_FINISHED_BATCH, 300);
	setDefault(GEOIP_AUTO_UPDATE, TRUE);
	setDefault(GEOIP_CHECK_HOURS, 30);
	setDefault(USE_CUSTOM_LOCATIONS, TRUE);

	// Logging (Ints)
	setDefault(LOG_MAIN_CHAT, TRUE);
	setDefault(LOG_PRIVATE_CHAT, TRUE);
	
	// Startup & shutdown
	setDefault(SHUTDOWN_TIMEOUT, 150);
	if (!Util::isLocalMode())
	{
		setDefault(REGISTER_URL_HANDLER, TRUE);
		setDefault(REGISTER_MAGNET_HANDLER, TRUE);
		setDefault(REGISTER_DCLST_HANDLER, TRUE);
	}
	setDefault(DETECT_PREVIEW_APPS, TRUE);

	// Colors & text styles
	setDefault(BACKGROUND_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_COLOR, RGB(0, 0, 0));
	setDefault(ERROR_COLOR, RGB(255, 0, 0));
	setDefault(TEXT_GENERAL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_GENERAL_FORE_COLOR, RGB(0, 0, 0));
	setDefault(TEXT_MYOWN_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_MYOWN_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_PRIVATE_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_PRIVATE_FORE_COLOR, RGB(0, 0, 0));
	setDefault(TEXT_SYSTEM_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SYSTEM_FORE_COLOR, RGB(164, 0, 128));
	setDefault(TEXT_SYSTEM_BOLD, TRUE);
	setDefault(TEXT_SERVER_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SERVER_FORE_COLOR, RGB(192, 0, 138));
	setDefault(TEXT_SERVER_BOLD, TRUE);
	setDefault(TEXT_TIMESTAMP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_TIMESTAMP_FORE_COLOR, RGB(0, 91, 182));
	setDefault(TEXT_MYNICK_BACK_COLOR, RGB(240, 255, 240));
	setDefault(TEXT_MYNICK_FORE_COLOR, RGB(0, 170, 0));
	setDefault(TEXT_MYNICK_BOLD, TRUE);
	setDefault(TEXT_NORMAL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_NORMAL_FORE_COLOR, RGB(80, 80, 80));
	setDefault(TEXT_FAV_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_FAV_FORE_COLOR, RGB(0, 128, 255));
	setDefault(TEXT_FAV_BOLD, TRUE);
	setDefault(TEXT_OP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_OP_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_OP_BOLD, TRUE);
	setDefault(TEXT_URL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_URL_FORE_COLOR, RGB(0, 102, 204));
	setDefault(TEXT_ENEMY_BACK_COLOR, RGB(244, 244, 244));
	setDefault(TEXT_ENEMY_FORE_COLOR, RGB(255, 128, 64));

	// User list colors
	setDefault(RESERVED_SLOT_COLOR, RGB(255, 0, 128));
	setDefault(IGNORED_COLOR, RGB(128, 128, 128));
	setDefault(FAVORITE_COLOR, RGB(0, 128, 255));
	setDefault(FAV_BANNED_COLOR, RGB(255, 128, 64));
	setDefault(NORMAL_COLOR, RGB(0, 0, 0));
	setDefault(FIREBALL_COLOR, RGB(0, 0, 0));
	setDefault(SERVER_COLOR, RGB(0, 0, 0));
	setDefault(PASSIVE_COLOR, RGB(67, 98, 154));
	setDefault(OP_COLOR, RGB(0, 128, 64));
	setDefault(CHECKED_COLOR, RGB(104, 32, 238));
	setDefault(CHECKED_FAIL_COLOR, RGB(204, 0, 0));

	// Other colors
	setDefault(DOWNLOAD_BAR_COLOR, RGB(0x36, 0xDB, 0x24));
	setDefault(UPLOAD_BAR_COLOR, RGB(0x00, 0xAB, 0xFD));
	setDefault(PROGRESS_BACK_COLOR, RGB(0xCC, 0xCC, 0xCC));
	setDefault(PROGRESS_SEGMENT_COLOR, RGB(0x52, 0xB9, 0x44));
	setDefault(COLOR_RUNNING, RGB(84, 130, 252));
	setDefault(COLOR_RUNNING_COMPLETED, RGB(255, 255, 0));
	setDefault(COLOR_DOWNLOADED, RGB(0, 255, 0));
	setDefault(BAN_COLOR, RGB(116, 154, 179));
	setDefault(FILE_SHARED_COLOR, RGB(114, 219, 139));
	setDefault(FILE_DOWNLOADED_COLOR, RGB(145, 194, 196));
	setDefault(FILE_CANCELED_COLOR, RGB(210, 168, 211));
	setDefault(FILE_FOUND_COLOR, RGB(255, 255, 0));
	setDefault(FILE_QUEUED_COLOR, RGB(186, 0, 42));
	setDefault(TABS_INACTIVE_BACKGROUND_COLOR, RGB(220, 220, 220));
	setDefault(TABS_ACTIVE_BACKGROUND_COLOR, RGB(255, 255, 255));
	setDefault(TABS_INACTIVE_TEXT_COLOR, RGB(82, 82, 82));
	setDefault(TABS_ACTIVE_TEXT_COLOR, RGB(0, 0, 0));
	setDefault(TABS_OFFLINE_BACKGROUND_COLOR, RGB(244, 166, 166));
	setDefault(TABS_OFFLINE_ACTIVE_BACKGROUND_COLOR, RGB(255, 183, 183));
	setDefault(TABS_UPDATED_BACKGROUND_COLOR, RGB(217, 234, 247));
	setDefault(TABS_BORDER_COLOR, RGB(157, 157, 161));

	// Assorted UI settings (Ints)
	setDefault(SHOW_INFOTIPS, TRUE);
	setDefault(USE_SYSTEM_ICONS, TRUE);
	setDefault(MDI_MAXIMIZED, TRUE);
	setDefault(TOGGLE_ACTIVE_WINDOW, TRUE);

	// Tab settings
	setDefault(MAX_TAB_ROWS, 7);
	setDefault(TAB_SIZE, 30);
	setDefault(TABS_SHOW_INFOTIPS, TRUE);
	setDefault(TABS_CLOSEBUTTONS, TRUE);
	//setDefault(NON_HUBS_FRONT, FALSE);
	setDefault(BOLD_FINISHED_DOWNLOADS, TRUE);
	setDefault(BOLD_FINISHED_UPLOADS, TRUE);
	setDefault(BOLD_QUEUE, TRUE);
	setDefault(BOLD_HUB, TRUE);
	setDefault(BOLD_PM, TRUE);
	setDefault(BOLD_SEARCH, TRUE);
	setDefault(BOLD_WAITING_USERS, TRUE);
	setDefault(HUB_URL_IN_TITLE, TRUE);

	// Toolbar settings (Ints)
	setDefault(TB_IMAGE_SIZE, 24);

	// Menu settings
#ifdef _WIN32
	bool useFlatMenuHeader = SysVersion::isOsWin8Plus();
	uint32_t menuHeaderColor = SysVersion::isOsWin11Plus() ? RGB(185, 220, 255) : RGB(0, 128, 255);
#else
	int useFlatMenuHeader = FALSE;
	uint32_t menuHeaderColor = RGB(0, 128, 255);
#endif
	setDefault(USE_CUSTOM_MENU, TRUE);
	setDefault(MENUBAR_TWO_COLORS, !useFlatMenuHeader);
	setDefault(MENUBAR_LEFT_COLOR, menuHeaderColor);
	setDefault(MENUBAR_RIGHT_COLOR, RGB(168, 211, 255));
	setDefault(MENUBAR_BUMPED, !useFlatMenuHeader);
	setDefault(UC_SUBMENU, TRUE);

	// Progressbar settings
	setDefault(SHOW_PROGRESS_BARS, TRUE);
	setDefault(PROGRESS_OVERRIDE_COLORS, TRUE);
	setDefault(PROGRESS_TEXT_COLOR_DOWN, RGB(0, 0, 0));
	setDefault(PROGRESS_TEXT_COLOR_UP, RGB(0, 0, 0));
	setDefault(PROGRESS_3DDEPTH, 3);
	setDefault(PROGRESSBAR_ODC_STYLE, FALSE);
	setDefault(PROGRESSBAR_ODC_BUMPED, TRUE);
	setDefault(STEALTHY_STYLE_ICO, TRUE);
	setDefault(TOP_DL_SPEED, 512);
	setDefault(TOP_UL_SPEED, 512);

	// Popup settings (Ints)
	setDefault(POPUP_TIME, 5);
	setDefault(POPUP_WIDTH, 200);
	setDefault(POPUP_HEIGHT, 90);
	setDefault(POPUP_TRANSPARENCY, 200);
	setDefault(POPUP_MAX_LENGTH, 120);
	setDefault(POPUP_BACKCOLOR, RGB(58, 122, 180));
	setDefault(POPUP_TEXTCOLOR, RGB(255, 255, 255));
	setDefault(POPUP_TITLE_TEXTCOLOR, RGB(255, 255, 255));
	setDefault(POPUP_ON_FAVORITE_CONNECTED, TRUE);
	setDefault(POPUP_ON_FAVORITE_DISCONNECTED, TRUE);
	setDefault(POPUP_ON_CHEATING_USER, TRUE);
	setDefault(POPUP_ON_DOWNLOAD_FINISHED, TRUE);
	setDefault(POPUP_ON_NEW_PM, TRUE);
	setDefault(POPUP_ON_FOLDER_SHARED, TRUE);
	setDefault(POPUP_PM_PREVIEW, TRUE);
	setDefault(POPUP_ONLY_WHEN_MINIMIZED, TRUE);

	// Sound settings (Int)
	setDefault(SOUNDS_DISABLED, TRUE);

	// Open on startup
	setDefault(OPEN_RECENT_HUBS, TRUE);
	setDefault(OPEN_PUBLIC_HUBS, TRUE);

	// Click actions
	setDefault(CHAT_DBLCLICK, 1);
	setDefault(MAGNET_ASK, TRUE);
	setDefault(MAGNET_ACTION, MAGNET_ACTION_SEARCH);
	setDefault(SHARED_MAGNET_ASK, TRUE);
	setDefault(SHARED_MAGNET_ACTION, MAGNET_ACTION_SEARCH);
	setDefault(DCLST_ASK, TRUE);
	setDefault(DCLST_ACTION, MAGNET_ACTION_SEARCH);

	// Confirmations
	setDefault(CONFIRM_EXIT, TRUE);
	setDefault(CONFIRM_DELETE, TRUE);
	setDefault(CONFIRM_HUB_REMOVAL, TRUE);
	setDefault(CONFIRM_HUBGROUP_REMOVAL, TRUE);
	setDefault(CONFIRM_USER_REMOVAL, TRUE);
	setDefault(CONFIRM_FINISHED_REMOVAL, TRUE);
	setDefault(CONFIRM_SHARE_FROM_SHELL, TRUE);
	setDefault(CONFIRM_CLEAR_SEARCH_HISTORY, TRUE);
	setDefault(CONFIRM_ADLS_REMOVAL, TRUE);

	// Media player
	setDefault(USE_MAGNETS_IN_PLAYERS_SPAM, TRUE);

	// Other settings, mostly useless
	//setDefault(REDUCE_PRIORITY_IF_MINIMIZED_TO_TRAY, TRUE);
	setDefault(DCLST_CREATE_IN_SAME_FOLDER, TRUE);
	setDefault(DCLST_INCLUDESELF, TRUE);
	
	// View visibility
	setDefault(SHOW_STATUSBAR, TRUE);
	setDefault(SHOW_TOOLBAR, TRUE);
	setDefault(SHOW_TRANSFERVIEW, TRUE);
	setDefault(SHOW_TRANSFERVIEW_TOOLBAR, TRUE);
	setDefault(SHOW_QUICK_SEARCH, TRUE);

	// Frames UI state (Ints)
	setDefault(TRANSFER_FRAME_SORT, 3); // COLUMN_FILENAME
	setDefault(TRANSFER_FRAME_SPLIT, 8000);
	setDefault(HUB_FRAME_SORT, 1); // COLUMN_NICK
	setDefault(SEARCH_FRAME_SORT, -3); // COLUMN_HITS, descending
	setDefault(DIRLIST_FRAME_SORT, 1); // COLUMN_FILENAME
	setDefault(DIRLIST_FRAME_SPLIT, 2500);
	setDefault(QUEUE_FRAME_SPLIT, 2500);
	setDefault(PUBLIC_HUBS_FRAME_SORT, -3); // COLUMN_USERS, descending
	setDefault(USERS_FRAME_SPLIT, 8000);
	setDefault(UPLOAD_QUEUE_FRAME_SPLIT, 2500);
	setDefault(SPY_FRAME_SORT, 4); // COLUMN_TIME	

	// Hub frame
	setDefault(AUTO_FOLLOW, TRUE);
	setDefault(FAV_SHOW_JOINS, TRUE);
	setDefault(PROMPT_HUB_PASSWORD, TRUE);
	setDefault(FILTER_MESSAGES, TRUE);
	setDefault(ENABLE_COUNTRY_FLAG, TRUE);
	setDefault(SHOW_CHECKED_USERS, TRUE);
	setDefault(HUB_POSITION, POS_RIGHT);
	setDefault(USER_THRESHOLD, 1000);
	setDefault(POPUP_PMS_HUB, TRUE);
	setDefault(POPUP_PMS_BOT, TRUE);
	setDefault(POPUP_PMS_OTHER, TRUE);

	// Chat frame
	setDefault(CHAT_BUFFER_SIZE, 25000);
	setDefault(CHAT_TIME_STAMPS, TRUE);
	setDefault(BOLD_MSG_AUTHOR, TRUE);
	setDefault(CHAT_PANEL_SHOW_INFOTIPS, TRUE);
	setDefault(SHOW_EMOTICONS_BTN, TRUE);
	setDefault(CHAT_ANIM_SMILES, TRUE);
	setDefault(SMILE_SELECT_WND_ANIM_SMILES, TRUE);
	setDefault(SHOW_SEND_MESSAGE_BUTTON, TRUE);
#ifdef BL_UI_FEATURE_BB_CODES
	setDefault(FORMAT_BB_CODES, TRUE);
	setDefault(FORMAT_BB_CODES_COLORS, TRUE);
	setDefault(SHOW_BBCODE_PANEL, TRUE);
	setDefault(FORMAT_BOT_MESSAGE, TRUE);
#endif
	setDefault(SHOW_MULTI_CHAT_BTN, TRUE);
	setDefault(SHOW_LINK_BTN, TRUE);
	setDefault(SHOW_FIND_BTN, TRUE);
	setDefault(CHAT_REFFERING_TO_NICK, TRUE);
	setDefault(STATUS_IN_CHAT, TRUE);
	setDefault(DISPLAY_CHEATS_IN_MAIN_CHAT, TRUE);
	setDefault(USE_CTRL_FOR_LINE_HISTORY, TRUE);
	setDefault(MULTILINE_CHAT_INPUT_BY_CTRL_ENTER, TRUE);

	// Search frame
	setDefault(SEARCH_HISTORY, 30);
	setDefault(CLEAR_SEARCH, TRUE);	
	setDefault(USE_SEARCH_GROUP_TREE_SETTINGS, TRUE);

	// Users frame
	setDefault(SHOW_IGNORED_USERS, -1);

	// Queue frame
	setDefault(QUEUE_FRAME_SHOW_TREE, TRUE);
	
	// Finished frame
	setDefault(SHOW_SHELL_MENU, TRUE);

	// Search Spy frame
	setDefault(SHOW_SEEKERS_IN_SPY_FRAME, TRUE);
	setDefault(LOG_SEEKERS_IN_SPY_FRAME, FALSE);

	// Settings dialog
	setDefault(REMEMBER_SETTINGS_PAGE, TRUE);

	// Main window size & position
	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);

	// Recents (Ints)
	setDefault(SAVED_SEARCH_TYPE, 0);
	setDefault(SAVED_SEARCH_SIZEMODE, 2);
	setDefault(SAVED_SEARCH_MODE, 1);

	setSearchTypeDefaults();

	for (int i = STR_FIRST; i < STR_LAST; ++i)
		strDefaults[i].shrink_to_fit();
}

bool SettingsManager::loadLanguage()
{
	auto path = Util::getLanguagesPath();
	auto name = get(LANGUAGE_FILE);
	if (name.empty())
	{
		// TODO: Determine language from user's locale
		if (Text::getDefaultCharset() == 1251)
			name = "ru-RU.xml";
		else
			name = "en-US.xml";
	}
	path += name;
	return ResourceManager::loadLanguage(path);
}

static bool isPathSetting(int setting)
{
	return setting == SettingsManager::LOG_DIRECTORY ||
	       setting == SettingsManager::TLS_PRIVATE_KEY_FILE ||
	       setting == SettingsManager::TLS_CERTIFICATE_FILE ||
	       setting == SettingsManager::TLS_TRUSTED_CERTIFICATES_PATH ||
	       (setting >= SettingsManager::SOUND_BEEPFILE && setting <= SettingsManager::SOUND_SEARCHSPY);
}

void SettingsManager::load(const string& fileName)
{
	string modulePath = Util::getExePath();
	if (!File::isAbsolute(modulePath) || modulePath.length() == 3) modulePath.clear();
	try
	{
		SimpleXML xml;
		const string fileData = File(fileName, File::READ, File::OPEN).read();
		xml.fromXML(fileData);
		xml.stepIn();
		if (xml.findChild("Settings"))
		{
			xml.stepIn();
			int i;
			for (i = STR_FIRST; i < STR_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if (xml.findChild(attr))
				{
					if (isPathSetting(i))
					{
						string path = xml.getChildData();
						pathFromRelative(path, modulePath);
						set(StrSetting(i), path);
					}
					else
						set(StrSetting(i), xml.getChildData());
				}
				xml.resetCurrentChild();
			}
			for (i = INT_FIRST; i < INT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if (xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			xml.stepOut();
		}
		xml.resetCurrentChild();
		if (xml.findChild("SearchTypes"))
		{
			try
			{
				g_searchTypes.clear();
				xml.stepIn();
				while (xml.findChild("SearchType"))
				{
					const string& extensions = xml.getChildData();
					if (extensions.empty())
					{
						continue;
					}
					const string& name = xml.getChildAttrib("Id");
					if (name.empty())
					{
						continue;
					}
					g_searchTypes[name] = Util::splitSettingAndLower(extensions);
				}
				xml.stepOut();
			}
			catch (const SimpleXMLException&)
			{
				setSearchTypeDefaults();
			}
		}
		xml.stepOut();
	}
	catch (const FileException&)
	{
		//dcassert(0);
	}
	catch (const SimpleXMLException&)
	{
		dcassert(0);
	}
	
	int excludePorts[3];
	int excludeCount = 0;
	int port = get(TCP_PORT);
	if (port) excludePorts[excludeCount++] = port;
	port = get(TLS_PORT);
	if (port) excludePorts[excludeCount++] = port;
	port = get(WEBSERVER_PORT);
	if (port) excludePorts[excludeCount++] = port;
	
	if (get(TCP_PORT) == 0)
	{
		port = generateRandomPort(excludePorts, excludeCount);
		set(TCP_PORT, port);
		excludePorts[excludeCount++] = port;
	}

	if (get(TLS_PORT) == 0)
	{
		port = generateRandomPort(excludePorts, excludeCount);
		set(TLS_PORT, port);
		excludePorts[excludeCount++] = port;
	}

	if (get(WEBSERVER_PORT) == 0)
	{
		port = generateRandomPort(excludePorts, excludeCount);
		set(WEBSERVER_PORT, port);
	}

	if (get(UDP_PORT) == 0)
	{
		port = generateRandomPort(excludePorts, 0);
		set(UDP_PORT, port);
	}

	if (SETTING(PRIVATE_ID).length() != 39 || CID(SETTING(PRIVATE_ID)).isZero())
		set(PRIVATE_ID, CID::generate().toBase32());
	
	if (SETTING(DHT_KEY).length() != 39 || CID(SETTING(DHT_KEY)).isZero())
		set(DHT_KEY, CID::generate().toBase32());

	if (get(WEBSERVER_PASS).empty())
		set(WEBSERVER_PASS, Util::getRandomPassword());
	if (get(WEBSERVER_POWER_PASS).empty())
		set(WEBSERVER_POWER_PASS, Util::getRandomPassword());

	//������� ����� ��������� �������
	if (strstr(get(TEMP_DOWNLOAD_DIRECTORY).c_str(), "%[targetdir]\\") != 0)
		set(TEMP_DOWNLOAD_DIRECTORY, "");
		
	const auto& path = SETTING(TLS_TRUSTED_CERTIFICATES_PATH);
	if (!path.empty())
		File::ensureDirectory(path);

	string& theme = strSettings[THEME_MANAGER_THEME_DLL_NAME - STR_FIRST];
	if (Text::isAsciiSuffix2<string>(theme, ".dll")) theme.erase(theme.length() - 4);
	if (Text::isAsciiSuffix2<string>(theme, "_x64")) theme.erase(theme.length() - 4);
}

void SettingsManager::loadOtherSettings()
{
	try
	{
		SimpleXML xml;
		xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());
		xml.stepIn();
		ShareManager::getInstance()->load(xml);
		auto fm = FavoriteManager::getInstance();
		fm->loadRecents(xml);
		fm->loadPreviewApps(xml);
		fm->loadSearchUrls(xml);

		fire(SettingsManagerListener::Load(), xml);
		xml.stepOut();
	}
	catch (const FileException&)
	{
	}
	catch (const SimpleXMLException&)
	{
	}
	ShareManager::getInstance()->init();
}

#define REDUCE_LENGTH(maxLength)\
	{\
		valueAdjusted = value.size() > maxLength;\
		strSettings[key - STR_FIRST] = valueAdjusted ? value.substr(0, maxLength) : value;\
	}
			
#define REPLACE_SPACES()\
	{\
		auto cleanValue = value;\
		boost::replace_all(cleanValue, " ", "");\
		strSettings[key - STR_FIRST] = cleanValue;\
	}

bool SettingsManager::set(StrSetting key, const std::string& value)
{
	bool valueAdjusted = false;

	switch (key)
	{
		case LOG_FORMAT_DOWNLOAD:
		case LOG_FORMAT_UPLOAD:
		case LOG_FORMAT_MAIN_CHAT:
		case LOG_FORMAT_PRIVATE_CHAT:
		case LOG_FORMAT_STATUS:
		case LOG_FORMAT_SYSTEM:
		case LOG_FORMAT_SQLITE_TRACE:
		case LOG_FORMAT_FLOOD_TRACE:
		case LOG_FORMAT_TCP_MESSAGES:
		case LOG_FORMAT_UDP_PACKETS:
		case LOG_FILE_MAIN_CHAT:
		case LOG_FILE_STATUS:
		case LOG_FILE_PRIVATE_CHAT:
		case LOG_FILE_UPLOAD:
		case LOG_FILE_DOWNLOAD:
		case LOG_FILE_SYSTEM:
		case LOG_FILE_WEBSERVER:
		case LOG_FILE_SQLITE_TRACE:
		case LOG_FILE_TORRENT_TRACE:
		case LOG_FILE_DHT_TRACE:
		case LOG_FILE_PSR_TRACE:
		case LOG_FILE_FLOOD_TRACE:
		case LOG_FILE_TCP_MESSAGES:
		case LOG_FILE_UDP_PACKETS:
		case TIME_STAMPS_FORMAT:
		{
			string newValue = value;
			if (key == LOG_FORMAT_MAIN_CHAT || key == LOG_FORMAT_PRIVATE_CHAT)
			{
				if (value.find(" [extra]") != string::npos ||
				    value.find("S[extra]") != string::npos ||
				    value.find("%H:%M%:%S") != string::npos)
				{
					boost::replace_all(newValue, " [extra]", " %[extra]");
					boost::replace_all(newValue, "S[extra]", "S %[extra]");
					boost::replace_all(newValue, "%H:%M%:%S", "%H:%M:%S");
				}
			}
			strSettings[key - STR_FIRST] = newValue;
			if (key == LOG_FILE_PRIVATE_CHAT)
			{
				const string templatePMFolder = "PM\\%B - %Y\\";
				if (newValue.find(templatePMFolder) != string::npos)
				{
					boost::replace_all(newValue, templatePMFolder, "PM\\%Y-%m\\");
					strSettings[key - STR_FIRST] = newValue;
				}
			}
			break;
		}
		case LOG_FILE_TLS_CERT:
		{
			if (value.find("%[kp]") == string::npos && value.find("%[KP]") == string::npos)
				strSettings[key - STR_FIRST].clear();
			else
				strSettings[key - STR_FIRST] = value;
			break;
		}
		case BIND_ADDRESS:
		case BIND_ADDRESS6:
		case WEBSERVER_BIND_ADDRESS:
		case URL_GET_IP:
		case URL_GET_IP6:
		case URL_DHT_BOOTSTRAP:
		case URL_PORT_TEST:
		{
			string trimmedValue = value;
			boost::algorithm::trim(trimmedValue);
			strSettings[key - STR_FIRST] = trimmedValue;
			break;
		}
		case AUTO_PRIORITY_PATTERNS:
			REPLACE_SPACES();
			break;
		case SKIPLIST_SHARE:
			REPLACE_SPACES();
			break;
		case NICK:
			REDUCE_LENGTH(49); // [~] InfinitySky. 35 -> 49
			break;
		case DESCRIPTION:
			REDUCE_LENGTH(100); // [~] InfinitySky. 50 -> 100
			break;
		case EMAIL:
			REDUCE_LENGTH(64);
			break;
		case DCLST_DIRECTORY:
		case DOWNLOAD_DIRECTORY:
		case TEMP_DOWNLOAD_DIRECTORY:
		case LOG_DIRECTORY:
		{
			string& path = strSettings[key - STR_FIRST];
			path = value;
			Util::appendPathSeparator(path);
			break;
		}
		default:
			strSettings[key - STR_FIRST] = value;
			break;
			
#undef REDUCE_LENGTH
#undef REPLACE_SPACES
	}
	
	if (valueAdjusted) // ���� �������� �������� � ������ �������� - ������ ������ ��� ����� ���������� ������� � ����.
	{
		isSet[key] = true;
	}
	else if (key >= SOUND_BEEPFILE && key <= SOUND_SEARCHSPY)
	{
		isSet[key] = true;
	}
	else
	{
		isSet[key] = !value.empty();
	}
	
	return valueAdjusted;
}

bool SettingsManager::set(IntSetting key, int value)
{
	bool valueAdjusted = false;
	switch (key)
	{

#define VER_MIN(min)\
	if (value < min)\
	{\
		value = min;\
		valueAdjusted = true;\
	}

#define VER_MAX(max)\
	if (value > max)\
	{\
		value = max;\
		valueAdjusted = true;\
	}

#define VER_DEF_VAL(min, max, defVal)\
	if (value < min || value > max)\
	{\
		value = defVal;\
		valueAdjusted = true;\
	}

#define VER_MIN_EXCL_ZERO(min)\
	if (value < min && value != 0)\
	{\
		value = min;\
		valueAdjusted = true;\
	}

#define VERIFY(min, max)\
	VER_MIN(min);\
	VER_MAX(max)

		case AUTO_SEARCH_LIMIT:
		{
			VER_MIN(1);
			break;
		}
		case BUFFER_SIZE_FOR_DOWNLOADS:
		{
			VERIFY(64, 8192); // 64Kb - 8Mb
			break;
		}
		case SLOTS:
		{
			VERIFY(0, 500);
			break;
		}
		case EXTRA_SLOTS:
		{
			VER_MIN(0);
			break;
		}
		case HUB_SLOTS:
		{
			VER_MIN(0);
			break;
		}
		case MINISLOT_SIZE:
		{
			VER_MIN(16);
			break;
		}
		case MAX_CHUNK_SIZE:
		{
			VER_MIN_EXCL_ZERO(64*1024);
			break;
		}
#ifdef OSVER_WIN_XP
		case SOCKET_IN_BUFFER:
		case SOCKET_OUT_BUFFER:
		{
			VERIFY(0, MAX_SOCKET_BUFFER_SIZE);
			break;
		}
#endif
		case NUMBER_OF_SEGMENTS:
		{
			VERIFY(1, 200);
			break;
		}
		case AUTO_SEARCH_TIME:
		{
			VERIFY(1, 60);
			break;
		}
		case MIN_SEARCH_INTERVAL:
		case MIN_SEARCH_INTERVAL_PASSIVE:
		{
			VER_DEF_VAL(2, 120, 10);
			break;
		}
		case DB_LOG_FINISHED_UPLOADS:
		case DB_LOG_FINISHED_DOWNLOADS:
		case MAX_FINISHED_UPLOADS:
		case MAX_FINISHED_DOWNLOADS:
		case MAX_HUB_USER_COMMANDS:
		{
			VER_MIN(0);
			break;
		}
		case DB_FINISHED_BATCH:
		{
			VERIFY(0, 2000);
			break;
		}
		case GEOIP_CHECK_HOURS:
		{
			VER_MIN(1);
			break;
		}
		case MYINFO_DELAY:
		{
			VERIFY(0, 180);
			break;
		}
		case POPUP_TIME:
		{
			VERIFY(1, 15);
			break;
		}
		case POPUP_MAX_LENGTH:
		{
			VERIFY(3, 512);
			break;
		}
		case POPUP_WIDTH:
		{
			VERIFY(80, 599);
			break;
		}
		case POPUP_HEIGHT:
		{
			VERIFY(50, 299);
			break;
		}
		case POPUP_TRANSPARENCY:
		{
			VERIFY(50, 255);
			break;
		}
		case TB_IMAGE_SIZE:
		{
			if (value != 24 && value != 16)
			{
				value = 24;
				valueAdjusted = true;
			}
			break;
		}
		case MAX_UPLOAD_SPEED_LIMIT_NORMAL:
		case MAX_DOWNLOAD_SPEED_LIMIT_NORMAL:
		case MAX_UPLOAD_SPEED_LIMIT_TIME:
		case MAX_DOWNLOAD_SPEED_LIMIT_TIME:
		case THROTTLE_ENABLE:
		{
			if (key != THROTTLE_ENABLE && value > 0 && value < MIN_SPEED_LIMIT)
			{
				value = MIN_SPEED_LIMIT;
				valueAdjusted = true;
			}
			intSettings[key - INT_FIRST] = value;
			isSet[key] = true;
			ThrottleManager::getInstance()->updateLimits();
			return valueAdjusted;
		}
		case IPUPDATE_INTERVAL:
		{
			VERIFY(0, 86400);
			if (value > 0 && value < 10)
			{
				value = 10;
				valueAdjusted = true;
			}
			break;
		}
		case MEDIA_PLAYER:
		{
			VERIFY(WinAmp, PlayersCount-1);
			break;
		}
		case TCP_PORT:
		case TLS_PORT:
		case UDP_PORT:
		{
			VER_MIN_EXCL_ZERO(1024);
			VER_MAX(65535);
			break;
		}
		case WEBSERVER_PORT:
		{
			VERIFY(1024, 65535);
			break;
		}
		case DHT_PORT:
		{
			VERIFY(1024, 65535);
			break;
		}
		case BANDWIDTH_LIMIT_START:
		case BANDWIDTH_LIMIT_END:
		{
			VERIFY(0, 23);
			break;
		}
		case PER_USER_UPLOAD_SPEED_LIMIT:
		{
			VERIFY(0, 10240);
			break;
		}
		case DIRLIST_FRAME_SPLIT:
		case UPLOAD_QUEUE_FRAME_SPLIT:
		case QUEUE_FRAME_SPLIT:
		{
			VERIFY(80, 8000);
			break;
		}
		case SEARCH_HISTORY:
		{
			VERIFY(10, 80);
			break;
		}
		case KEEP_LISTS_DAYS:
		{
			VERIFY(0, 9999);
			break;
		}
		case PM_LOG_LINES:
		{
			VERIFY(0, 999);
			break;
		}
		case CCPM_IDLE_TIMEOUT:
		{
			VERIFY(0, 30);
			break;
		}
		case SHUTDOWN_ACTION:
		{
			VERIFY(0, 5);
			break;
		}
		case SHUTDOWN_TIMEOUT:
		{
			VERIFY(1, 3600);
			break;
		}
		case SQLITE_JOURNAL_MODE:
		{
			VERIFY(0, 3);
			break;
		}
		case USER_CHECK_BATCH:
		{
			VERIFY(5, 50);
			break;
		}
#undef VER_MIN
#undef VER_MAX
#undef VER_DEF_VAL
#undef VER_MIN_EXCL_ZERO
#undef VERIFY
	}
	intSettings[key - INT_FIRST] = value;
	isSet[key] = true;
	return valueAdjusted;
}

void SettingsManager::save(const string& fileName)
{
	string modulePath = Util::getExePath();
	if (!File::isAbsolute(modulePath) || modulePath.length() == 3) modulePath.clear();

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();
	
	int i;
	string type("type"), curType("string");
	
	for (i = STR_FIRST; i < STR_LAST; i++)
	{
		if (i == CONFIG_VERSION)
		{
			xml.addTag(settingTags[i], VERSION_STR);
			xml.addChildAttrib(type, curType);
		}
		else if (isSet[i])
		{
			if (isPathSetting(i))
			{
				string path = get(StrSetting(i), false);
				pathToRelative(path, modulePath);
				xml.addTag(settingTags[i], path);
			}
			else
				xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	
	curType = "int";
	for (i = INT_FIRST; i < INT_LAST; i++)
	{
		if (isSet[i])
		{
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	FavoriteManager::getInstance()->savePreviewApps(xml);
	FavoriteManager::getInstance()->saveSearchUrls(xml);
	ShareManager::getInstance()->saveShareList(xml);

	fire(SettingsManagerListener::Save(), xml);

	if (!ClientManager::isBeforeShutdown())
	{
		fire(SettingsManagerListener::Repaint());
	}
	
	try
	{
		string tempFile = fileName + ".tmp";
		File out(tempFile, File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out, 1024);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flushBuffers(true);
		out.close();
		File::renameFile(tempFile, fileName);
	}
	catch (const FileException& e)
	{
		LogManager::message("error create/write .xml file:" + fileName + " error = " + e.getError());
	}
}

void SettingsManager::validateSearchTypeName(const string& name)
{
	if (name.empty() || (name.size() == 1 && name[0] >= '1' && name[0] <= '6'))
	{
		throw SearchTypeException("Invalid search type name"); // TODO translate
	}
	for (int type = FILE_TYPE_ANY; type != NUMBER_OF_FILE_TYPES; ++type)
	{
		if (STRING_I(SearchManager::getTypeStr(type)) == name)
		{
			throw SearchTypeException("This search type already exists"); // TODO translate
		}
	}
}

void SettingsManager::setSearchTypeDefaults()
{
	g_searchTypes.clear();
	
	// for conveniency, the default search exts will be the same as the ones defined by SEGA.
	const auto& searchExts = AdcHub::getSearchExts();
	for (size_t i = 0, n = searchExts.size(); i < n; ++i)
		g_searchTypes[string(1, (char) ('1' + i))] = searchExts[i];
		
}

void SettingsManager::addSearchType(const string& name, const StringList& extensions, bool validated)
{
	if (!validated)
	{
		validateSearchTypeName(name);
	}
	
	if (g_searchTypes.find(name) != g_searchTypes.end())
	{
		throw SearchTypeException("This search type already exists"); // TODO translate
	}
	
	g_searchTypes[name] = extensions;
}

void SettingsManager::delSearchType(const string& name)
{
	validateSearchTypeName(name);
	g_searchTypes.erase(name);
}

void SettingsManager::renameSearchType(const string& oldName, const string& newName)
{
	validateSearchTypeName(newName);
	StringList exts = getSearchType(oldName)->second;
	addSearchType(newName, exts, true);
	g_searchTypes.erase(oldName);
}

void SettingsManager::modSearchType(const string& name, const StringList& extensions)
{
	getSearchType(name)->second = extensions;
}

const StringList& SettingsManager::getExtensions(const string& name)
{
	return getSearchType(name)->second;
}

SettingsManager::SearchTypesIter SettingsManager::getSearchType(const string& name)
{
	auto ret = g_searchTypes.find(name);
	if (ret == g_searchTypes.end())
	{
		throw SearchTypeException("No such search type"); // TODO translate
	}
	return ret;
}

int SettingsManager::generateRandomPort(const int* exclude, int excludeCount)
{
	int value;
	for (;;)
	{
		value = Util::rand(49152, 65536);
		bool excluded = false;
		for (int i = 0; i < excludeCount && !excluded; ++i)
			if (value == exclude[i])
				excluded = true;
		if (!excluded) break;
	}
	return value;
}

string SettingsManager::getSoundFilename(const SettingsManager::StrSetting sound)
{
	if (getBool(SOUNDS_DISABLED, true))
		return Util::emptyString;
		
	return get(sound, true);
}

bool SettingsManager::getBeepEnabled(const SettingsManager::IntSetting sound)
{
	return !getBool(SOUNDS_DISABLED, true) && getBool(sound, true);
}

bool SettingsManager::getPopupEnabled(const SettingsManager::IntSetting popup)
{
	return !getBool(POPUPS_DISABLED, true) && getBool(popup, true);
}

const string& SettingsManager::get(StrSetting key, const bool useDefault /*= true*/)
{
	if (isSet[key] || !useDefault)
		return strSettings[key - STR_FIRST];
	else
		return strDefaults[key - STR_FIRST];
}

int SettingsManager::get(IntSetting key, const bool useDefault /*= true*/)
{
	if (isSet[key] || !useDefault)
		return intSettings[key - INT_FIRST];
	else
		return intDefaults[key - INT_FIRST];
}

const string& SettingsManager::getDefault(StrSetting key)
{
	return strDefaults[key - STR_FIRST];
}

int SettingsManager::getDefault(IntSetting key)
{
	return intDefaults[key - INT_FIRST];
}

bool SettingsManager::set(IntSetting key, const std::string& value)
{
	if (value.empty())
	{
		intSettings[key - INT_FIRST] = 0;
		isSet[key] = false;
		return false;
	}
	return set(key, Util::toInt(value));
}

void SettingsManager::pathToRelative(string& path, const string& prefix)
{
	if (!prefix.empty() && File::isAbsolute(path) && prefix.length() < path.length() && !strnicmp(path, prefix, prefix.length()))
		path.erase(0, prefix.length());
}

void SettingsManager::pathFromRelative(string& path, const string& prefix)
{
	if (path.empty() || File::isAbsolute(path) || path[0] == PATH_SEPARATOR || prefix.empty()) return;
	if (prefix.back() != PATH_SEPARATOR) path.insert(0, PATH_SEPARATOR_STR);
	path.insert(0, prefix);
}

void SettingsManager::getIPSettings(IPSettings& s, bool v6)
{
	if (v6)
	{
		s.autoDetect = AUTO_DETECT_CONNECTION6;
		s.manualIp = WAN_IP_MANUAL6;
		s.noIpOverride = NO_IP_OVERRIDE6;
		s.incomingConnections = INCOMING_CONNECTIONS6;
		s.bindAddress = BIND_ADDRESS6;
		s.bindDevice = BIND_DEVICE6;
		s.bindOptions = BIND_OPTIONS6;
		s.externalIp = EXTERNAL_IP6;
		s.mapper = MAPPER6;
	}
	else
	{
		s.autoDetect = AUTO_DETECT_CONNECTION;
		s.manualIp = WAN_IP_MANUAL;
		s.noIpOverride = NO_IP_OVERRIDE;
		s.incomingConnections = INCOMING_CONNECTIONS;
		s.bindAddress = BIND_ADDRESS;
		s.bindDevice = BIND_DEVICE;
		s.bindOptions = BIND_OPTIONS;
		s.externalIp = EXTERNAL_IP;
		s.mapper = MAPPER;
	}
}

int SettingsManager::getIdByName(const string& name) noexcept
{
	for (int i = 0; i < SETTINGS_LAST; i++)
		if (name == settingTags[i]) return i;
	return -1;
}

string SettingsManager::getNameById(int id) noexcept
{
	return settingTags[id];
}

void SettingsManager::load()
{
	Util::migrate(getConfigFile());
	load(getConfigFile());
}

void SettingsManager::save()
{
	save(getConfigFile());
}
