// Competition Entry
//   Played around with assembly a bit.
//
// Topic:FOOD
// Name: Boldly Going
//
// Do not forget to include DDRAW.LIB, DINPUT.LIB, DINPUT8.LIB, DSOUND.LIB, WINMM.LIB

// INCLUDES ///////////////////////////////////////////////

#define INITGUID
#define WIN32_LEAN_AND_MEAN

#include<windows.h>
#include<windowsx.h>
#include<mmsystem.h>
#include<objbase.h>
#include<iostream>
#include<io.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<math.h>
#include<malloc.h>
#include<memory.h>
#include<string.h>
#include<stdarg.h>
#include<conio.h>
#include"resource.h"

#include<ddraw.h>
#include<dinput.h>
#include<dsound.h>
#include<dmusici.h>
#include<dmusicc.h>
#include<dmusicf.h>
#include<dmksctrl.h>

// DEFINES ////////////////////////////////////////////////

#define TITLE		"Boldly Going"
#define CLASS		TITLE
#define WIDTH		640
#define HEIGHT		480
#define BPP			16

#define GAME_STATE_TITLE		0
#define GAME_STATE_INTRO		1
#define GAME_STATE_MENU			2
#define GAME_STATE_RUN			3

#define MAXFOOD					256
#define MAXBULLET				50
#define RATE					(float)(.001f)
#define MAX						75

// MACROS /////////////////////////////////////////////////

// msg box for errors
#define MSG(msg) MessageBox(main_window_handle,msg,"Error!",MB_OK|MB_ICONEXCLAMATION)

// used just for quick exit
#define KEY_DOWN(n) (keyboard_state[n] & 0x80)

// builds 15 bit color
#define _RGB16BIT555(r,g,b) ((b & 31) + ((g & 31) << 5) + ((r & 31) << 10))

// builds 16 bit color
#define _RGB16BIT565(r,g,b) ((b & 31) + ((g & 63) << 5) + ((r & 31) << 11))

// initialize a ddstruct
#define DDRAW_INIT_STRUCT(ddstruct) {memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct);}

// TYPES //////////////////////////////////////////////////

typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

class Bitmap
{
public:
	int End();
	int Load(char *filename,int sx,int sy,int swidth,int sheight,int mem_flags=DDSCAPS_SYSTEMMEMORY);
	int Draw();
	int x;
	int y;
	int width;
	int height;
	LPDIRECTDRAWSURFACE7 image;
};

class Bullets
{
public:
	void Draw();
	void Initiate();
	int x;
	int y;
	int active;
};

class Food
{
public:
	void Draw();
	void Initiate();
	int x;
	int y;
	int yv;
	int active;
	int food;
};

// PROTOTYPES /////////////////////////////////////////////

int Game_Init(void);
int Game_Main(void);
int Game_Shutdown(void);
int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,USHORT color);
int Draw_Text(char *text,int x,int y,COLORREF color,LPDIRECTDRAWSURFACE7 lpdds);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle = NULL;
HINSTANCE main_instance = NULL;
int fps = 0; // used to keep track of Frames Per Second
int index=0; // used for looping
int keystate[256]; // used to keep track of keypress/up
int game_state=GAME_STATE_TITLE;
int game_title_to_intro = 0;
char buffer[256];
int space_state;
int cheat_test;
DWORD score;
int missed;
int bullet_switch;
int endgame=0;
float diff=1;

// DirectX globals
LPDIRECTDRAW7 lpdd = NULL;
LPDIRECTDRAWSURFACE7 lpddsprimary = NULL;
LPDIRECTDRAWSURFACE7 lpddsback = NULL;
DDSURFACEDESC2 ddsd;
DDSCAPS2 ddscaps;
UCHAR *back_buffer;
int back_lpitch;
UCHAR *primary_buffer;
int primary_lpitch;
int dd_pixel_format=BPP;
LPDIRECTINPUT8 lpdi = NULL;
LPDIRECTINPUTDEVICE8 lpdikey = NULL;
LPDIRECTINPUTDEVICE8 lpdimouse = NULL;
UCHAR keyboard_state[256];
DIMOUSESTATE mouse_state;

Food food[MAXFOOD];
Bullets bullets[MAXBULLET];

// Bitmaps
Bitmap title;
Bitmap menu;
Bitmap background;
Bitmap ship;
Bitmap foodimage[7];
Bitmap bullet;

// FUNCTIONS //////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	// Just check to make sure we shouldn't quit
	switch(msg)
	{
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return(0);
		}break;
	default:break;
	}

	return(DefWindowProc(hwnd,msg,wparam,lparam));
}
///////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hinstance,HINSTANCE hprevinstance,
				   LPSTR lpcmdline,int ncdmshow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg;

	// fill wc structure
	wc.cbSize=sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS|CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(hinstance,MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.lpszClassName = CLASS;
	wc.lpszMenuName = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	// register
	if(!RegisterClassEx(&wc))
		return(0); // BIG error

	// create window
	if(!(hwnd=CreateWindowEx(NULL,CLASS,TITLE,WS_POPUP|WS_VISIBLE,0,0,WIDTH,HEIGHT,NULL,NULL,hinstance,NULL)))
		return(0); // yet another BIG error

	// move window information to globals
	main_window_handle = hwnd;
	main_instance = hinstance;

	// Hide the cursor and initiate the game
	ShowCursor(FALSE);
	Game_Init();

	while(1)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			if(msg.message==WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Set to 33 fps
		srand(fps=GetTickCount());

		// read keyboard
		if(lpdikey)
		{
			if(lpdikey->GetDeviceState(256,(LPVOID)keyboard_state)!=DI_OK)
				exit(0);
		}
		else
		{
			// not plugged in
			memset(keyboard_state,0,sizeof(keyboard_state));
			MSG("You need to plugin your keyboard");
		}

		// read mouse
		if(FAILED(lpdimouse->GetDeviceState(sizeof(DIMOUSESTATE),(LPVOID)&mouse_state)))
			exit(0);

		// Playing the game
		Game_Main();

		// wait, do nothing, la da dee da da daaa
		while((GetTickCount()-fps)<30);

		// flip it!
		while(FAILED(lpddsprimary->Flip(NULL,DDFLIP_WAIT)));

	}

	// Shutting down and showing the cursor
	Game_Shutdown();
	ShowCursor(TRUE);

	return(msg.wParam);
}
///////////////////////////////////////////////////////////
int Game_Init(void)
{
	// create DDraw interface
	if(FAILED(DirectDrawCreateEx(NULL,(void **)&lpdd,IID_IDirectDraw7,NULL)))
		exit(0);

	// set cooperation level to fullscreen
	if(FAILED(lpdd->SetCooperativeLevel(main_window_handle,
			DDSCL_ALLOWMODEX|DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE|
			DDSCL_ALLOWREBOOT|DDSCL_MULTITHREADED)))
		exit(0);

	// set the display mode
	if(FAILED(lpdd->SetDisplayMode(WIDTH,HEIGHT,BPP,0,0)))
		_exit(0);

	// set data for primary surface
	memset(&ddsd,0,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);

	// create complex flippable surface struct
	ddsd.dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount=1;

	// create primary surface
	lpdd->CreateSurface(&ddsd,&lpddsprimary,NULL);

	// get pixel format(15 or 16)
	DDPIXELFORMAT ddpf;

	// initialize struct
	DDRAW_INIT_STRUCT(ddpf);

	// query format from primary
	lpddsprimary->GetPixelFormat(&ddpf);

	// test 6 bit green mask
	if(ddpf.dwGBitMask==0x000007E0)
	{
		dd_pixel_format=16;
	}
	else // must be 15 bit
	{
		dd_pixel_format=15;
	}

	// we needs us a backbuffer
	ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
	if(FAILED(lpddsprimary->GetAttachedSurface(&ddscaps,&lpddsback)))
		exit(0);

	// clear both buffers
	DDraw_Fill_Surface(lpddsprimary,0);
	DDraw_Fill_Surface(lpddsback,0);

	// Setup DirectInput
	if(FAILED(DirectInput8Create(main_instance,DIRECTINPUT_VERSION,IID_IDirectInput8,(void **)&lpdi,NULL)))
		exit(0);

	// create keyboard
	if(lpdi->CreateDevice(GUID_SysKeyboard,&lpdikey,NULL)!=DI_OK)
		exit(0);

	// set cooperation level
	if(lpdikey->SetCooperativeLevel(main_window_handle,DISCL_NONEXCLUSIVE|DISCL_BACKGROUND)!=DI_OK)
		exit(0);

	// set data format
	if(lpdikey->SetDataFormat(&c_dfDIKeyboard)!=DI_OK)
		exit(0);

	// acquire keyboard
	if(lpdikey->Acquire()!=DI_OK)
		exit(0);

	// do the same for the mouse
	if(lpdi->CreateDevice(GUID_SysMouse,&lpdimouse,NULL)!=DI_OK)
		exit(0);

	// set coop...
	if(lpdimouse->SetCooperativeLevel(main_window_handle,DISCL_NONEXCLUSIVE|DISCL_BACKGROUND)!=DI_OK)
		exit(0);

	// set data format
	if(lpdimouse->SetDataFormat(&c_dfDIMouse)!=DI_OK)
		exit(0);

	// acquire
	if(lpdimouse->Acquire()!=DI_OK)
		exit(0);

	// create resources here
	title.Load("Media\\Title",0,0,640,480,DDSCAPS_SYSTEMMEMORY);
	menu.Load("Media\\Menu",0,0,640,480,DDSCAPS_SYSTEMMEMORY);
	background.Load("Media\\Background",0,0,640,480,DDSCAPS_SYSTEMMEMORY);
	ship.Load("Media\\ShipFood",1,1,50,100,DDSCAPS_SYSTEMMEMORY);
		_asm mov ship.x,295 //ship.x=295;
		_asm mov ship.y,380 //ship.y=380;
	bullet.Load("Media\\ShipFood",52,1,3,6,DDSCAPS_SYSTEMMEMORY);
	foodimage[0].Load("Media\\ShipFood",1,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[1].Load("Media\\ShipFood",22,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[2].Load("Media\\ShipFood",43,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[3].Load("Media\\ShipFood",64,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[4].Load("Media\\ShipFood",85,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[5].Load("Media\\ShipFood",106,102,20,20,DDSCAPS_SYSTEMMEMORY);
	foodimage[6].Load("Media\\ShipFood",127,102,20,20,DDSCAPS_SYSTEMMEMORY);

	return(1);
}
///////////////////////////////////////////////////////////
int Game_Main(void)
{
	// clear surface
	DDraw_Fill_Surface(lpddsback,0);
	
	// check to see if game is over
	if(endgame)
	{
		sprintf(buffer,"HEARTBURN! GAME OVER! SCORE: %d! Press Esc!",score);
		Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,235,RGB(255,255,255),lpddsprimary);
		endgame=0;
		game_state=GAME_STATE_MENU;
		while(!KEY_DOWN(DIK_ESCAPE))
		{
			// read keyboard
			if(lpdikey)
			{
				if(lpdikey->GetDeviceState(256,(LPVOID)keyboard_state)!=DI_OK)
					exit(0);
			}
			else
			{
				// not plugged in
				memset(keyboard_state,0,sizeof(keyboard_state));
				MSG("You need to plugin your keyboard");
			}
		}
		while(KEY_DOWN(DIK_ESCAPE))
		{
			// read keyboard
			if(lpdikey)
			{
				if(lpdikey->GetDeviceState(256,(LPVOID)keyboard_state)!=DI_OK)
					exit(0);
			}
			else
			{
				// not plugged in
				memset(keyboard_state,0,sizeof(keyboard_state));
				MSG("You need to plugin your keyboard");
			}
		}
	}


	switch(game_state)
	{
	case GAME_STATE_RUN:
		{
			// do game here
			// draw background
			background.Draw();

			// draw ship
			ship.Draw();

			//score
			sprintf(buffer,"SCORE: %d",score);
			Draw_Text(buffer,0,15,RGB(255,255,255),lpddsback);

			//number missed
			sprintf(buffer,"MISSED: %d",missed);
			Draw_Text(buffer,0,30,RGB(255,255,255),lpddsback);

			//randomly do food
			if((diff+=RATE)>MAX)
				diff=MAX;

			if((rand()%(MAX-(int)diff+2)<=2))
			{
				for(index=0;index<MAXFOOD;index++)
				{
					if(!food[index].active)
					{
						food[index].Initiate();
						goto foodisokay;
					}
				}
			}
foodisokay: _asm nop

			// take care of food
			for(index=0;index<MAXFOOD;index++)
			{
				if(food[index].active)
				{
					food[index].y+=food[index].yv;
					if(food[index].y>(HEIGHT-20))
					{
						food[index].active=0;
						_asm inc missed //missed++;
						if(missed>=100)
						{
							endgame=1;
						}
						goto endfoodcheck;
					}
					food[index].Draw();
				}
endfoodcheck:
				_asm nop
			}

			// take care of bullets
			for(index=0;index<MAXBULLET;index++)
			{
				if(bullets[index].active)
				{
					bullets[index].y-=6;
					if(bullets[index].y<0)
					{
						bullets[index].active=0;
						goto endbulletcheck;
					}
					bullets[index].Draw();
				}
endbulletcheck:
				_asm nop
			}

			// Collsion detect
			for(int bulletindex=0;bulletindex<MAXBULLET;bulletindex++)
			{
				if(bullets[bulletindex].active)
				{
					for(int foodindex=0;foodindex<MAXFOOD;foodindex++)
					{
						if(food[foodindex].active)
						{
							int cx1=bullets[bulletindex].x+((3>>1)-(3>>3));
							int cy1=bullets[bulletindex].y+((6>>1)-(6>>3));
							int cx2=food[foodindex].x+((20>>1)-(20>>3));
							int cy2=food[foodindex].y+((20>>1)-(20>>3));
							int dx=abs(cx2-cx1);
							int dy=abs(cy2-cy1);
							if(dx<((3>>1)-(3>>3)+(20>>1)-(20>>3)) &&
							dy<((6>>1)-(6>>3)+(20>>1)-(20>>3)))
							{
								//// we have a collision
								food[foodindex].active=0;
								bullets[bulletindex].active=0;
								score++;
							}
						}
					}
				}
			}

			// CHEAT CODE!!!
			if(KEY_DOWN(DIK_N)&&KEY_DOWN(DIK_B)&&!cheat_test)
			{
				cheat_test=1;
				score=score/2;
				for(index=0;index<MAXFOOD;index++)
					food[index].active=0;
			}

			// check to see if left mouse button is hit
			if(mouse_state.rgbButtons[0] & 0x80)
			{
				if(space_state != 0)
				{
					//if(space_state++==15)
					_asm inc space_state
					if(space_state==15)
					{
						_asm mov space_state,0 //space_state=0;
					}
				}
				else
				{
					for(index=0;index<MAXBULLET;index++)
					{
						if(!bullets[index].active)
						{
							bullets[index].Initiate();
							space_state=1;
							goto bulletisokay;
						}
					}
				}
			}
			else
			{
				space_state=0;
			}
bulletisokay: _asm nop

			// move ship
			ship.x+=mouse_state.lX;
		    
			// check left side
			if(ship.x<0)
				ship.x=0;

			// check right side
			if(ship.x>590)
				ship.x=590;

			// if user is pressing "esc" leave
			if(KEY_DOWN(DIK_ESCAPE))
			{
				keystate[DIK_ESCAPE]=1;
			}
			else if(keystate[DIK_ESCAPE])
			{
				memset(keystate,0,sizeof(keystate));
				game_state=GAME_STATE_MENU;
			}
		}break;
	case GAME_STATE_MENU:
		{
			// do menu here
			// draw menu screen
			menu.Draw();

			// if user is pressing "esc" leave
			if(KEY_DOWN(DIK_SPACE))
			{
				keystate[DIK_SPACE]=1;
			}
			else if(keystate[DIK_SPACE])
			{
				memset(keystate,0,sizeof(keystate));
				game_state = GAME_STATE_RUN;
				// reset
				for(index = 0;index<MAXBULLET;index++)
				{
					bullets[index].active=0;
					bullets[index].x=0;
					bullets[index].y=0;
				}
				ship.x=295;
				for(index=0;index<MAXFOOD;index++)
				{
					food[index].active=0;
					food[index].food=0;
					food[index].x=0;
					food[index].y=0;
					food[index].yv=0;
				}
				score=0;
				space_state=0;
				missed=0;
				endgame=0;
				cheat_test=0;
			}

			// if user is pressing "esc" leave
			if(KEY_DOWN(DIK_ESCAPE))
			{
				keystate[DIK_ESCAPE]=1;
			}
			else if(keystate[DIK_ESCAPE])
			{
				memset(keystate,0,sizeof(keystate));
				PostMessage(main_window_handle,WM_DESTROY,0,0);
			}

		}break;
	case GAME_STATE_INTRO:
		{
			// Do intro here
			// static ints
			static int wait1,wait2,wait3,wait4,wait5,waiter;
			if(wait1<252 && waiter!=1)
			{
				wait1+=4;
			}
			else if(wait2<252 && waiter!=1)
			{
				wait2+=4;
			}
			else if(wait3<252 && waiter!=1)
			{
				wait3+=4;
			}
			else if(wait4<252 && waiter!=1)
			{
				wait4+=4;
			}
			else if(wait5<252 && waiter!=1)
			{
				wait5+=4;
			}
			else if(wait5>250 && waiter!=1)
			{
				waiter=1;
			}
			else if(waiter==1)
			{
				wait1-=4;
				wait2-=4;
				wait3-=4;
				wait4-=4;
				if(wait1<5)
				{
					game_state=GAME_STATE_MENU;
				}
			}

			// print messages
			sprintf(buffer,"In the year 2004...");
			Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,71,RGB(wait1,wait1,wait1),lpddsback);
			sprintf(buffer,"A courageous adventurer embarked on a journey...");
			Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,152,RGB(wait2,wait2,wait2),lpddsback);
			sprintf(buffer,"B O L D L Y   G O I N G");
			Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,233,RGB(wait3,wait3,wait3),lpddsback);
			sprintf(buffer,"Where no one has before ventured...");
			Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,314,RGB(wait4,wait4,wait4),lpddsback);
			sprintf(buffer,"The stomach...");
			Draw_Text(buffer,(640-(strlen(buffer)*7.5))/2,395,RGB(wait5,wait5,wait5),lpddsback);


			// test to see if user is pressing escape key
			if(KEY_DOWN(DIK_ESCAPE))
			{
				keystate[DIK_ESCAPE]=1;
			}
			else if(keystate[DIK_ESCAPE])
			{
				memset(keystate,0,sizeof(keystate));
				game_state=GAME_STATE_MENU;
			}
		}break;
	case GAME_STATE_TITLE:
		{
			// Draw Title screen
			title.Draw();

			// show for a little bit
			if((game_title_to_intro++)==60)
			{
				game_title_to_intro=0;
				game_state=GAME_STATE_INTRO;
				break;
			}

			// test to see if user is pressing escape key
			if(KEY_DOWN(DIK_ESCAPE))
			{
				keystate[DIK_ESCAPE]=1;
			}
			else if(keystate[DIK_ESCAPE])
			{
				memset(keystate,0,sizeof(keystate));
				game_state=GAME_STATE_MENU;
			}

		}break;
	default:game_state=GAME_STATE_TITLE;
		break;
	}

	// make sure no one sells this
	sprintf(buffer,"© Copyright 2004, Luke Thompson, a.k.a. nerd_boy");
	Draw_Text(buffer,0,0,RGB(255,255,255),lpddsback);

	return(1);
}
///////////////////////////////////////////////////////////
int Game_Shutdown(void)
{
	// release resources
	title.End();
	menu.End();
	background.End();
	ship.End();
	foodimage[0].End();
	foodimage[1].End();
	foodimage[2].End();
	foodimage[3].End();
	foodimage[4].End();

	// shutdown DInput

	if(lpdikey)
	{
		lpdikey->Unacquire();
		lpdikey->Release();
	}

	if(lpdi)
		lpdi->Release();

	// shutdown DDraw surfaces
	if(lpddsback)
		lpddsback->Release();

	if(lpddsprimary)
		lpddsprimary->Release();

	if(lpdd)
		lpdd->Release();

	return(1);
}
///////////////////////////////////////////////////////////
int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,USHORT color)
{
	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.dwFillColor=color;
	lpdds->Blt(NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&ddbltfx);
	return(1);
}
///////////////////////////////////////////////////////////
Bitmap::Load(char *filename,int sx,int sy,int swidth,int sheight,int mem_flags)
{
	// set vars to appropriate values
	x=sx;
	y=sy;
	width=swidth;
	height=sheight;
	image=NULL;

	// vars needed to load file
	int file_handle;
	UCHAR *tempbuff = NULL,*buffer = NULL;
	OFSTRUCT file_data;
	BITMAPINFOHEADER bitmapinfoheader;
	BITMAPFILEHEADER bitmapfileheader;

	// open the file IF it exists
	if((file_handle=OpenFile(filename,&file_data,OF_READ))==-1)
		exit(0);

	// load bitmap file header
	_lread(file_handle,&bitmapfileheader,sizeof(BITMAPFILEHEADER));

	// test to see if this is a bitmap
	if(bitmapfileheader.bfType!=0x4D42)// universal BMP ID
	{
		_lclose(file_handle);
		exit(0);
	}

	// load bitmap info header
	_lread(file_handle,&bitmapinfoheader,sizeof(BITMAPINFOHEADER));

	// get image itself
	_lseek(file_handle,-(int)(bitmapinfoheader.biSizeImage),SEEK_END);

	if(bitmapinfoheader.biBitCount!=24)
	{
		// we only load 24 bit bitmaps
		_lclose(file_handle);
		exit(0);
	}

	// allocate tempbuff
	if(!(tempbuff=(UCHAR *)malloc(bitmapinfoheader.biSizeImage)))
	{
		_lclose(file_handle);
		exit(0);
	}

	// allocate actual buffer
	if(!(buffer=(UCHAR *)malloc(2*bitmapinfoheader.biWidth*bitmapinfoheader.biHeight)))
	{
		_lclose(file_handle);
		free(tempbuff);
		exit(0);
	}

	// read file in
	_lread(file_handle,tempbuff,bitmapinfoheader.biSizeImage);

	// convert 24 to 16 using our convert-o-thingie
	for(int tempindex=0;tempindex<bitmapinfoheader.biWidth*bitmapinfoheader.biHeight;tempindex++)
	{
		USHORT color;
		if(dd_pixel_format==15)
		{
			UCHAR blue = (tempbuff[tempindex*3+0]>>3),
				  green = (tempbuff[tempindex*3+1]>>3),
				  red = (tempbuff[tempindex*3+2]>>3);
			color=_RGB16BIT555(red,green,blue);
		}
		else // it is 565
		{
			UCHAR blue = (tempbuff[tempindex*3+0]>>3),
				  green = (tempbuff[tempindex*3+1]>>2),
				  red = (tempbuff[tempindex*3+2]>>3);
			color=_RGB16BIT565(red,green,blue);
		}
		// write to buffer
		((USHORT *)buffer)[tempindex]=color;
	}
	// free memory
	bitmapinfoheader.biBitCount=16;
	free(tempbuff);

	// close file
	_lclose(file_handle);

	// flip bitmap
	UCHAR *flipbuff;

	// allocate flipbuff
	if(!(flipbuff=(UCHAR *)malloc(bitmapinfoheader.biWidth*(bitmapinfoheader.biBitCount/8)*bitmapinfoheader.biHeight)))
		exit(0);

	// copy image to work area
	memcpy(flipbuff,buffer,(bitmapinfoheader.biWidth*(bitmapinfoheader.biBitCount/8)*bitmapinfoheader.biHeight));

	// flip vertically
	for(int i=0;i<bitmapinfoheader.biHeight;i++)
		memcpy(&buffer[((bitmapinfoheader.biHeight-1)-i)*bitmapinfoheader.biWidth*(bitmapinfoheader.biBitCount/8)],
		       &flipbuff[i*bitmapinfoheader.biWidth*(bitmapinfoheader.biBitCount/8)],bitmapinfoheader.biWidth*(bitmapinfoheader.biBitCount/8));

	// release memory
	free(flipbuff);

	// Create image surface
	DDSURFACEDESC2 ddsdtemp;

	// set to access caps,width,height
	memset(&ddsdtemp,0,sizeof(ddsdtemp));
	ddsdtemp.dwSize = sizeof(ddsdtemp);
	ddsdtemp.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	ddsdtemp.dwWidth = width;
	ddsdtemp.dwHeight = height;
	// set to offscreen plain surface
	ddsdtemp.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|mem_flags;

	// create actual surface
	if(FAILED(lpdd->CreateSurface(&ddsdtemp,&image,NULL)))
		exit(0);

	// set 0,0,0 to transparency
	DDCOLORKEY color_key;
	color_key.dwColorSpaceHighValue =
		color_key.dwColorSpaceLowValue = 0;

	// set color key for srcblt-ing
	image->SetColorKey(DDCKEY_SRCBLT,&color_key);

	// load data into image
	USHORT *source,*dest;
	
	// extract bitmap data
	source = (USHORT *)buffer+sy*bitmapinfoheader.biWidth+sx;

	// set size of struct
	ddsdtemp.dwSize=sizeof(ddsdtemp);

	// lock surface
	image->Lock(NULL,&ddsdtemp,DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR,NULL);

	// assign pointer to mem surface
	dest = (USHORT *)ddsdtemp.lpSurface;

	// iterate through scanline and copy bitmap
	for(int iy=0;iy<height;iy++)
	{
		memcpy(dest,source,(width*2));
		dest+=(ddsdtemp.lPitch>>1);
		source+=bitmapinfoheader.biWidth;
	}


	// unlock surface
	image->Unlock(NULL);

	return(1);
}
///////////////////////////////////////////////////////////
Bitmap::Draw()
{
	RECT dest,source;

	// fill in dest rect
	dest.left=x;
	dest.top=y;
	dest.right=x+width;
	dest.bottom=y+height;

	// fill in source rect
	source.left=0;
	source.top=0;
	source.right=width;
	source.bottom=height;

	// blt it
	lpddsback->Blt(&dest,image,&source,(DDBLT_WAIT|DDBLT_KEYSRC),NULL);

	return(1);
}
///////////////////////////////////////////////////////////
Bitmap::End()
{
	if(image)
		image->Release();
	return(1);
}
///////////////////////////////////////////////////////////
int Draw_Text(char *text,int x,int y,COLORREF color,LPDIRECTDRAWSURFACE7 lpdds)
{
	HDC xdc;

	// get dc
	if(FAILED(lpdds->GetDC(&xdc)))
		exit(0);

	// set colors for text
	SetTextColor(xdc,color);

	// set background to transparent
	SetBkMode(xdc,TRANSPARENT);

	// draw the text
	TextOut(xdc,x,y,text,strlen(text));

	// release
	lpdds->ReleaseDC(xdc);

	return(1);
}
///////////////////////////////////////////////////////////
void Food::Initiate(void)
{
	active=1;
	food=rand()%7;
	x=rand()%620;
	y=0;
	yv=4;
}
///////////////////////////////////////////////////////////
void Food::Draw(void)
{
	foodimage[food].x=x;
	foodimage[food].y=y;
	foodimage[food].Draw();
}
///////////////////////////////////////////////////////////
void Bullets::Initiate(void)
{
	y=ship.y+30;
	if(bullet_switch)
	{
		x=ship.x+3;
		bullet_switch=0;
	}
	else
	{
		x=ship.x+46;
		bullet_switch=1;
	}
	active=1;
}
///////////////////////////////////////////////////////////
void Bullets::Draw(void)
{
	bullet.x=x;
	bullet.y=y;
	bullet.Draw();
}
///////////////////////////////////////////////////////////