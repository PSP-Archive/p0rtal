//Codename: Portal 2D maker
//Programmed for the PSP Genesis Competition
//SinkeWS May, 2011

//This code is free to be modified or used in another application but
//on a condition that proper credits are given.

//Includes
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspkernel.h>
//#include <pspdebug.h>
#include <pspgu.h>
#include <png.h>
#include <stdio.h>
#include "graphics.h"
#include "tiledata.h"


//Defines
#define true 1
#define false 0

#define left 0
#define right 1
#define up 2
#define down 3

//Function declarations
int exit_callback(int arg1, int arg2, void *common);
int CallbackThread(SceSize args, void *argp);
int SetupCallbacks(void);

void MenuFunction(void);
void TilesetFunction(void);
void GameFunction(void);
void EditorFunction(void);
void CalculateTrajectory(void);
void SetDefaults(void);
void SetEditorDefaults(void);
void CopySampleToCurrent(int SampleIndex);
void CopyEditedToCurrent(void);

//Define the module info section
PSP_MODULE_INFO("p0rtal 2d: maker", 0, 1, 1);

//Use all available memory for this application (psplink png decompression fix)
PSP_HEAP_SIZE_MAX();

//Define the main thread's attribute value
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

//Valiables
struct GameCharacter
{
	float X;
	float Y;
	float AccelerationX;
	float AccelerationY;
	int InAir;
	int Steps;
	int Direction;
};

struct GamePortal
{
	int X;
	int Y;
	int Rotation;
	int Visible;
};

struct GamePointer
{
	float X;
	float Y;
	int Frame;
	int Degree;
	int LineVisible;
};

enum SceneSelector {SceneIntro, SceneDisclaimer, SceneSetMenu, SceneMenu, SceneTileset, SceneGame, SceneEditor};
enum TilesetIndex {TlsBlank, TlsMoon, TlsMetal, TlsLava, TlsLauncher, TlsStart, TlsEnd};
enum ErrorMessages {OK, ERROR, NOSCENE};

int CurrentScene = SceneIntro;
int IntroWait = 0;
int DisclaimerWait = 0;
int BulletX = 0;
int BulletY = 0;
int OrangePortalDelay = 0;
int BluePortalDelay = 0;
int ShootDelay = 0;
int GameOver = false;		//Death
int LevelOver = false;		//Beaten level
int CurrentMenuSelection = 0;
int SampleLevelIndex = 0;
int EngineIndex = 0;		//Engine launcher (0 - samples, 1 - editor)

//Values for editor
int EditorSelectorX = 0;
int EditorSelectorY = 0;
int EditorSelectedItem = 0;

struct GameCharacter MainCharacter;
struct GamePortal OrangePortal;
struct GamePortal BluePortal;
struct GamePortal LaunchPad;
struct GamePortal EndPoint;
struct GamePointer PortalPointer;
char TempString[200];

//Image data
Image *BackgroundImage;				//Selected background image
Image *TilesetImage;				//A 16x16 tileset
Image *TilesetFramebuffer;			//Framebuffer for tileset (for collision checking)
Image *CharacterImage;				//Game character
Image *PointerImage;				//30 frame portal pointer
Image *GenesisImage;				//The competition logo
Image *MenuImage;					//Background for main menu
Image *EditorFramebuffer;

//Gamepad
SceCtrlData GamePad;
int DPADTimeout = 0;
int ButtonsTimeout = 0;
int SelectFlag = false;

int main(int argc, char *argv[])
{
	//Init stuff
	SetupCallbacks();
	initGraphics();
	sceDisplayWaitVblankStart();
	
	//Create images
	TilesetFramebuffer = createImage(480, 272);
	EditorFramebuffer = createImage(480, 272);
	
	//Load images to memory
	BackgroundImage = loadImage("images/background_1.png");
	TilesetImage = loadImage("images/tileset.png");
	CharacterImage = loadImage("images/character.png");
	PointerImage = loadImage("images/pointer.png");
	GenesisImage = loadImage("images/genesis.png");
	MenuImage = loadImage("images/title_screen.png");
	
	//Intro should be shown for 5 seconds
	int IntroWait = 300;
	
	//Disclaimer should be shown for 10 seconds
	int DisclaimerWait = 600;
		
	//Main game loop
	while(true)
	{
		//Clear the screen
		clearScreen(0xFF000000);
		
		//Read gamepad
		sceCtrlReadBufferPositive(&GamePad, 1);

		//Go to a specifice scene
		switch(CurrentScene)
		{
			default:
				return NOSCENE;
				break;
				
			case SceneIntro:
				blitImageToScreen(0 ,0 ,480 , 272, GenesisImage, 0, 0);
				IntroWait--;
				if(IntroWait == 0) CurrentScene++;
				break;
				
			case SceneDisclaimer:
				printTextScreen(10 ,10 , "Portal is a trademark of Valve Corporation.", 0xFFFFFFFF);
				printTextScreen(10 ,20 , "This software is a fan made product.", 0xFFFFFFFF);
				printTextScreen(10 ,40 , "All logos and trademarks are property", 0xFFFFFFFF);
				printTextScreen(10 ,50 , "of their respective owners.", 0xFFFFFFFF);
				DisclaimerWait--;
				if(DisclaimerWait == 0) CurrentScene++;
				break;
			
			case SceneSetMenu:		//Load default values and stuff
				CurrentMenuSelection = 0;
				SampleLevelIndex = 0;
				GameOver = false;
				LevelOver = false;
				CurrentScene++;
				break;
			
			case SceneMenu:
				MenuFunction();
				break;
				
			case SceneTileset:
				SetDefaults();
				TilesetFunction();
				break;
				
			case SceneGame:
				GameFunction();
				break;
				
			case SceneEditor:
				EditorFunction();
				break;
		}
		
		//Wait for VBlank
		sceDisplayWaitVblank();
		
		//Flip main and backbuffer
		flipScreen();
	}
	
	sceKernelSleepThread();
	return OK;
}

//Main menu of the game
void MenuFunction(void)
{

	if(GamePad.Buttons & PSP_CTRL_UP)
	{
		CurrentMenuSelection = 0;
	}
		
	if(GamePad.Buttons & PSP_CTRL_DOWN)
	{
		CurrentMenuSelection = 1;
	}
	
	if(GamePad.Buttons & PSP_CTRL_CROSS)
	{
		//Play sample levels
		if(CurrentMenuSelection == 0)
		{
			CurrentScene = SceneTileset;
			EngineIndex = 0;
		}
		
		//Open level editor
		if(CurrentMenuSelection == 1)
		{
			CurrentScene = SceneEditor;
			EngineIndex = 1;
			SetEditorDefaults();		//Load default values
		}
	}
	
	//Draw background
	blitImageToScreen(0 ,0 ,480 , 272, MenuImage, 0, 0);
	
	//Draw menu
	printTextScreen(165 ,170 , "Play sample levels", 0xFFFFFFFF);
	printTextScreen(180 ,185 , "Create a level", 0xFFFFFFFF);
	
	//Draw menu selector
	printTextScreen(150 ,170 + (CurrentMenuSelection * 15), "[", 0xFF0000FF);
	printTextScreen(316 ,170 + (CurrentMenuSelection * 15), "]", 0xFF0000FF);
		
	printTextScreen(12 ,252 , "Code: SinkeWS, Graphics: Cettah, Sample levels: AxisZ8008", 0xFFFFFFFF);
}

//Draw tileset to tileset framebuffer
void TilesetFunction(void)
{
	int X = 0;
	int Y = 0;

	//Clear tileset framebuffer
	clearImage(0x00000000, TilesetFramebuffer);
	
	//Load sample level (if play samples was launched)
	if(EngineIndex == 0 && SampleLevelIndex < 5)
	{
		if(LevelOver == true)SampleLevelIndex ++;
		CopySampleToCurrent(SampleLevelIndex);
	}
	
	if(EngineIndex == 1)CopyEditedToCurrent();
	
	//Get background image name
	sprintf(TempString, "images/background_%d.png", GamePlayfield[510] + 1);
	
	//Free old background image
	freeImage(BackgroundImage);
	
	//Load new background image
	BackgroundImage = loadImage(TempString);
	
	//Draw playfield
	for(Y = 0; Y < 17; Y++)
	{
		for(X = 0; X < 30; X++)
		{
			switch(GamePlayfield[Y*30 + X])
			{
				default:
				case TlsBlank:
					//Don't draw anything
					break;
					
				case TlsMoon:
					blitImageToImage(16, 0, 16, 16, TilesetImage, X*16, Y*16, TilesetFramebuffer);
					break;
					
				case TlsMetal:
					blitImageToImage(32, 0, 16, 16, TilesetImage, X*16, Y*16, TilesetFramebuffer);
					break;
					
				case TlsLava:
					blitImageToImage(48, 0, 16, 16, TilesetImage, X*16, Y*16, TilesetFramebuffer);
					break;
					
				case TlsLauncher:
					LaunchPad.X = X * 16;
					LaunchPad.Y = Y * 16;
					LaunchPad.Visible = true;
					break;
				
				case TlsStart:
					MainCharacter.X = X * 16;
					MainCharacter.Y = Y * 16;
					break;
					
				case TlsEnd:
					EndPoint.X = X * 16;
					EndPoint.Y = Y * 16;
					EndPoint.Visible = true;
					break;
			}
		}
	}
	
	//Overlay tileset over background
	blitAlphaImageToImage(0 ,0 ,480 , 272, TilesetFramebuffer, 0, 0, BackgroundImage);
	
	CurrentScene++;
	
	//Go to menu if the end of samples was reached
	if(EngineIndex == 0 && SampleLevelIndex == 5) CurrentScene = SceneSetMenu;
	
	//Go to editor if editor is active
	if(EngineIndex == 1 && LevelOver == true)
	{
		CurrentScene = SceneEditor;
		
		//Reset background
		freeImage(BackgroundImage);
		BackgroundImage = loadImage("images/background_1.png");
	}
}


//Active part of the game
void GameFunction(void)
{
	//char TempString[200];			//Debug info
	int i = 0;						//Counter
	int TempX = 0;					//Temp X varialble
	int TempY = 0;					//Temp Y variable
	int OldX = 0;					//Old X variable
	int OldY = 0;					//Old Y variable
	int OldRotation = 0;			//Old rotation var
	int OldVisible = 0;				//Old visible var
	float OldAccelerationX = 0;
	float OldAccelerationY = 0;
	
	//Reset game states
	GameOver = false;
	LevelOver = false;
	
	//Check DPAD timeout
	if (DPADTimeout == 0)
	{
		if(GamePad.Buttons & PSP_CTRL_RIGHT)
		{
			if(MainCharacter.AccelerationX < 2)MainCharacter.AccelerationX = 2;
			MainCharacter.Direction = right;
		}
	
		if(GamePad.Buttons & PSP_CTRL_LEFT)
		{
			if(MainCharacter.AccelerationX > -2)MainCharacter.AccelerationX = -2;
			MainCharacter.Direction = left;
		}
	}
	
	//Set target line to unvisible
	PortalPointer.LineVisible = false;
	
	if(GamePad.Buttons & PSP_CTRL_RTRIGGER)
	{
		PortalPointer.Degree += 8;
		PortalPointer.LineVisible = true;
	}
		
	if(GamePad.Buttons & PSP_CTRL_LTRIGGER)
	{
		PortalPointer.Degree -= 8;
		PortalPointer.LineVisible = true;
	}
	
	//Check if cross is pressed, jump if it isn't
	if((GamePad.Buttons & PSP_CTRL_CROSS) && MainCharacter.InAir == false)
	{
		MainCharacter.AccelerationY = -2.2;
		MainCharacter.InAir = true;
	}
	
	if(GamePad.Buttons & PSP_CTRL_SELECT && SelectFlag == false)
	{
		//Break game, simulate level over scenario
		LevelOver = true;
		SelectFlag = true;
	}
	
	//Reset select flag if select is not pressed
	if((GamePad.Buttons & PSP_CTRL_SELECT) == false) SelectFlag = false;
	
	//Place blue portal
	if((GamePad.Buttons & PSP_CTRL_SQUARE) && ShootDelay == 0)
	{
		OldX = BluePortal.X;
		OldY = BluePortal.Y;
		OldRotation = BluePortal.Rotation;
		OldVisible = BluePortal.Visible;
		
		TempX = (BulletX / 16) * 16;
		TempY = (BulletY / 16) * 16;

		if((getPixelImage(TempX, TempY, TilesetFramebuffer) == 0xFFFFFFFF))
		{
			BluePortal.X = (BulletX / 16) * 16;
			BluePortal.Y = (BulletY / 16) * 16;
			BluePortal.Visible = true;
			ShootDelay = 15;
			
			if((BulletX == TempX) || (BulletX == TempX + 15))
			{
				if((MainCharacter.X + 8) < BulletX)
				{
					BluePortal.Rotation = left;
					BluePortal.X -= 16;
				}
				else
				{
					BluePortal.Rotation = right;
					BluePortal.X += 16;
				}
			}
			else
			{
				if((MainCharacter.Y + 8) > BulletY)
				{
					BluePortal.Y += 16;
					BluePortal.Rotation = up;
				}
				else 
				{
					BluePortal.Y -= 16;
					BluePortal.Rotation = down;
				}
			}
			
			if(BluePortal.X == OrangePortal.X && BluePortal.Y == OrangePortal.Y)
			{
				BluePortal.X = OldX;
				BluePortal.Y = OldY;
				BluePortal.Rotation = OldRotation;
				BluePortal.Visible = OldVisible;
			}
		}
		
		//Fix portals stuck inside an object
		if(getPixelImage(BluePortal.X, BluePortal.Y, TilesetFramebuffer) != 0)
		{
			BluePortal.X = OldX;
			BluePortal.Y = OldY;
			BluePortal.Rotation = OldRotation;
			BluePortal.Visible = OldVisible;
		}
	}
	
	//Place orange portal
	if((GamePad.Buttons & PSP_CTRL_CIRCLE) && ShootDelay == 0)
	{
		OldX = OrangePortal.X;
		OldY = OrangePortal.Y;
		OldRotation = OrangePortal.Rotation;
		OldVisible = OrangePortal.Visible;
		ShootDelay = 15;
					
		TempX = (BulletX / 16) * 16;
		TempY = (BulletY / 16) * 16;

		if((getPixelImage(TempX, TempY, TilesetFramebuffer) == 0xFFFFFFFF))
		{
			OrangePortal.X = (BulletX / 16) * 16;
			OrangePortal.Y = (BulletY / 16) * 16;
			OrangePortal.Visible = true;
			
			if((BulletX == TempX) || (BulletX == TempX + 15))
			{
				if((MainCharacter.X + 8) < BulletX)
				{
					OrangePortal.Rotation = left;
					OrangePortal.X -= 16;
				}
				else
				{
					OrangePortal.Rotation = right;
					OrangePortal.X += 16;
				}
			}
			else
			{
				if((MainCharacter.Y + 8) > BulletY)
				{
					OrangePortal.Y += 16;
					OrangePortal.Rotation = up;
				}
				else 
				{
					OrangePortal.Y -= 16;
					OrangePortal.Rotation = down;
				}
			}
			
			if(BluePortal.X == OrangePortal.X && BluePortal.Y == OrangePortal.Y)
			{
				OrangePortal.X = OldX;
				OrangePortal.Y = OldY;
				OrangePortal.Rotation = OldRotation;
				OrangePortal.Visible = OldVisible;
			}
		}
		
		//Fix portals stuck inside an object
		if(getPixelImage(OrangePortal.X, OrangePortal.Y, TilesetFramebuffer) != 0)
		{
			OrangePortal.X = OldX;
			OrangePortal.Y = OldY;
			OrangePortal.Rotation = OldRotation;
			OrangePortal.Visible = OldVisible;
		}
	}
	
	//Check right if charater is moving right
	if(MainCharacter.X < 464 && MainCharacter.AccelerationX > 0)
	{
		for(i = 0; i<16; i++)
			if(getPixelImage(MainCharacter.X + 15 + MainCharacter.AccelerationX, MainCharacter.Y + i, TilesetFramebuffer) != 0)
				MainCharacter.AccelerationX = 0;
	}
	
	//Check left if charater is moving left
	if(MainCharacter.X > 0 && MainCharacter.AccelerationX < 0)
	{
		for(i = 0; i<16; i++)
			if(getPixelImage(MainCharacter.X + MainCharacter.AccelerationX, MainCharacter.Y + i, TilesetFramebuffer) != 0)
				MainCharacter.AccelerationX = 0;
	}
	
	//Apply character movement
	if(MainCharacter.AccelerationY < 8)MainCharacter.AccelerationY += 0.1;
	
	//Check down if charater is moving down
	if(MainCharacter.Y < 256 && MainCharacter.AccelerationY > 0)
	{
		for(i = 0; i<16; i++)
			if(getPixelImage(MainCharacter.X + i, MainCharacter.Y + 15 + MainCharacter.AccelerationY, TilesetFramebuffer) != 0)
				{
					MainCharacter.AccelerationY = 0;
					MainCharacter.InAir = false;
				}
	}
	
	//Check up if charater is moving up
	if(MainCharacter.Y > 0 && MainCharacter.AccelerationY < 0)
	{
		for(i = 0; i<16; i++)
			if(getPixelImage(MainCharacter.X + i, MainCharacter.Y + MainCharacter.AccelerationY, TilesetFramebuffer) != 0)
				{
					MainCharacter.AccelerationY = 0;
					MainCharacter.InAir = true;
				}
	}

	//Decrease character X acceleration
	if(MainCharacter.AccelerationX > 0)MainCharacter.AccelerationX -= 0.1;
	if(MainCharacter.AccelerationX < 0)MainCharacter.AccelerationX += 0.1;
		
	//Fix sliding issue
	if(MainCharacter.AccelerationX > 0 && MainCharacter.AccelerationX < 0.2) MainCharacter.AccelerationX = 0;
	
	MainCharacter.X += MainCharacter.AccelerationX;
	MainCharacter.Y += MainCharacter.AccelerationY;
	
	//Fix collisions
	if(MainCharacter.X < 0)MainCharacter.X = 0;
	if(MainCharacter.X > 464)MainCharacter.X = 464;
	if(MainCharacter.Y < 0)
	{
		MainCharacter.Y = 0;
		MainCharacter.AccelerationY = 0;
	}
	if(MainCharacter.Y > 256)
	{
		MainCharacter.Y = 256;
		MainCharacter.AccelerationY = 0;
		MainCharacter.InAir = false;
	}
	
	//Calculate pointer coordinates
	if(PortalPointer.Degree < 0) PortalPointer.Degree = 1432;
	if(PortalPointer.Degree > 1432) PortalPointer.Degree = 0;
	
	if(PortalPointer.Degree < 232)
	{
		PortalPointer.X = PortalPointer.Degree + 232;
		PortalPointer.Y = 0;
	}
	
	if(PortalPointer.Degree > 232)
	{
		PortalPointer.X = 464;
		PortalPointer.Y = PortalPointer.Degree - 232;
	}
	
	if(PortalPointer.Degree > 488)
	{
		PortalPointer.X = 464 - (PortalPointer.Degree - 488);
		PortalPointer.Y = 256;
	}
	
	if(PortalPointer.Degree > 944)
	{
		PortalPointer.X = 0;
		PortalPointer.Y = 256 - (PortalPointer.Degree - 944);
	}
	
	if(PortalPointer.Degree > 1200)
	{
		PortalPointer.X = PortalPointer.Degree - 1200;
		PortalPointer.Y = 0;
	}
	
	//Calculate bullet trajectory
	CalculateTrajectory();
	
	//Decrease portal delay
	if(OrangePortalDelay > 0)OrangePortalDelay --;
	if(BluePortalDelay > 0)BluePortalDelay --;
	
	//Decrease shoot delay
	if(ShootDelay > 0)ShootDelay --;
	
	//Load acceleration
	OldAccelerationX = MainCharacter.AccelerationX;
	OldAccelerationY = MainCharacter.AccelerationY;
	
	//Check for orange portal
	if(OrangePortal.Visible == true && BluePortal.Visible == true && OrangePortalDelay == 0)
	{
		if((OrangePortal.X - MainCharacter.X+8) >= 0 && (OrangePortal.X - MainCharacter.X+8) < 16)
		{
			if((OrangePortal.Y - MainCharacter.Y+8) >= 0 && (OrangePortal.Y - MainCharacter.Y+8) < 16)
			{
				MainCharacter.X = BluePortal.X;
				MainCharacter.Y = BluePortal.Y;
				MainCharacter.InAir = true;
				BluePortalDelay = 15;

				if((BluePortal.Rotation == up && OrangePortal.Rotation == up) || (BluePortal.Rotation == down && OrangePortal.Rotation == down))
				{
					MainCharacter.AccelerationY = - OldAccelerationY;
				}

				if((BluePortal.Rotation == left && OrangePortal.Rotation == left) || (BluePortal.Rotation == right && OrangePortal.Rotation == right))
				{
					MainCharacter.AccelerationX = - OldAccelerationX;
				}
				
				if(OrangePortal.Rotation == up || OrangePortal.Rotation == down)
				{
					if(BluePortal.Rotation == left)
					{
						if(OldAccelerationY > 0)MainCharacter.AccelerationX -= OldAccelerationY;
						else MainCharacter.AccelerationX += OldAccelerationY;
						MainCharacter.AccelerationY = 0;
					}
					
					if(BluePortal.Rotation == right)
					{
						if(OldAccelerationY < 0)MainCharacter.AccelerationX -= OldAccelerationY;
						else MainCharacter.AccelerationX += OldAccelerationY;
						MainCharacter.AccelerationY = 0;
					}
				}
			}
		}
	}
	
	//Load acceleration
	OldAccelerationX = MainCharacter.AccelerationX;
	OldAccelerationY = MainCharacter.AccelerationY;
	
	//Check for blue portal
	if(OrangePortal.Visible == true && BluePortal.Visible == true && BluePortalDelay == 0)
	{
		if((BluePortal.X - MainCharacter.X+8) >= 0 && (BluePortal.X - MainCharacter.X+8) < 16)
		{
			if((BluePortal.Y - MainCharacter.Y+8) >= 0 && (BluePortal.Y - MainCharacter.Y+8) < 16)
			{
				MainCharacter.X = OrangePortal.X;
				MainCharacter.Y = OrangePortal.Y;
				MainCharacter.InAir = true;
				OrangePortalDelay = 15;
				
				if((OrangePortal.Rotation == up && BluePortal.Rotation == up) || (OrangePortal.Rotation == down && BluePortal.Rotation == down))
				{
					MainCharacter.AccelerationY = - OldAccelerationY;
				}
				
				if((OrangePortal.Rotation == left && BluePortal.Rotation == left) || (OrangePortal.Rotation == right && BluePortal.Rotation == right))
				{
					MainCharacter.AccelerationX = - OldAccelerationX;
				}
				
				if(BluePortal.Rotation == up || BluePortal.Rotation == down)
				{
					if(OrangePortal.Rotation == left)
					{
						if(OldAccelerationY > 0)MainCharacter.AccelerationX -= OldAccelerationY;
						else MainCharacter.AccelerationX += OldAccelerationY;
						
						MainCharacter.AccelerationY = 0;
					}
					
					if(OrangePortal.Rotation == right)
					{
						if(OldAccelerationY < 0)MainCharacter.AccelerationX -= OldAccelerationY;
						else MainCharacter.AccelerationX += OldAccelerationY;
						
						MainCharacter.AccelerationY = 0;
					}
				}
			}
		}
	}
	
	//Check for LaunchPad
	if(LaunchPad.Visible == true)
	{
		if((LaunchPad.X - MainCharacter.X+8) >= 0 && (LaunchPad.X - MainCharacter.X+8) < 16)
		{
			if((LaunchPad.Y - MainCharacter.Y+8) >= 0 && (LaunchPad.Y - MainCharacter.Y+8) < 16)
			{
				MainCharacter.AccelerationY = -3;
			}
		}
	}
	
	//Check for ending point
	if(EndPoint.Visible == true)
	{
		if((EndPoint.X - MainCharacter.X+8) >= 0 && (EndPoint.X - MainCharacter.X+8) < 16)
		{
			if((EndPoint.Y - MainCharacter.Y+8) >= 0 && (EndPoint.Y - MainCharacter.Y+8) < 16)
			{
				LevelOver = true;
			}
		}
	}
	
	//Check for lava(aka game over)
	if(MainCharacter.X > 0)
	{
		if(getPixelImage(MainCharacter.X - 1, MainCharacter.Y, TilesetFramebuffer) == 0xFF314F3A)GameOver = true;
	}
	
	if(MainCharacter.X < 460)
	{
		if(getPixelImage(MainCharacter.X + 17, MainCharacter.Y, TilesetFramebuffer) == 0xFF314F3A)GameOver = true;
	}
	
	if(MainCharacter.Y > 0)
	{
		if(getPixelImage(MainCharacter.X, MainCharacter.Y - 1, TilesetFramebuffer) == 0xFF314F3A)GameOver = true;
	}
	
	if(MainCharacter.Y < 250)
	{
		if(getPixelImage(MainCharacter.X, MainCharacter.Y + 17, TilesetFramebuffer) == 0xFF314F3A)GameOver = true;
	}
	
	//Fix in air var
	if(MainCharacter.AccelerationY != 0) MainCharacter.InAir = true;
	
	//Animate character
	if (MainCharacter.AccelerationX != 0 && MainCharacter.Steps < 40)MainCharacter.Steps++;
	if (MainCharacter.AccelerationX == 0 || MainCharacter.Steps==40)MainCharacter.Steps = 0;
	
	//Animate pointer
	if(PortalPointer.Frame < 60)PortalPointer.Frame ++;
	if(PortalPointer.Frame == 60)PortalPointer.Frame = 0;
	
	//Draw background
	blitImageToScreen(0 ,0 ,480 , 272, BackgroundImage, 0, 0);

	//Draw portals
	if(OrangePortal.Visible == true)blitAlphaImageToScreen(OrangePortal.Rotation * 16, 32, 16, 16, TilesetImage, OrangePortal.X, OrangePortal.Y);
	if(BluePortal.Visible == true)blitAlphaImageToScreen(BluePortal.Rotation * 16, 16, 16, 16, TilesetImage, BluePortal.X, BluePortal.Y);
	
	//Draw launch pad
	if(LaunchPad.Visible == true)blitAlphaImageToScreen(64, 0, 16, 16, TilesetImage, LaunchPad.X, LaunchPad.Y);
		
	//Draw end point
	if(EndPoint.Visible == true)blitImageToScreen(96, 0, 16, 16, TilesetImage, EndPoint.X, EndPoint.Y);
	
	//Draw "bullet" line
	if(PortalPointer.LineVisible == true)drawLineScreen(MainCharacter.X + 8, MainCharacter.Y + 8, BulletX, BulletY, 0xFF283AFF);
	else drawLineScreen(MainCharacter.X + 8, MainCharacter.Y + 8, BulletX, BulletY, 0xFF202744);
	
	//Draw main character
	blitAlphaImageToScreen(16 * (MainCharacter.Steps / 10), (16 * MainCharacter.Direction) + (MainCharacter.InAir * 32), 16, 16, CharacterImage, MainCharacter.X, MainCharacter.Y);

	//Draw pointer
	blitAlphaImageToScreen((PortalPointer.Frame / 2) * 16, 0, 16, 16, PointerImage, PortalPointer.X, PortalPointer.Y);
	
	//Build information
	//printTextScreen(10 ,10 , "Preview build, not a final product.", 0xFFFFFFFF);
	
	//Debug information
	//sprintf(TempString, "X acceleration: %f", MainCharacter.AccelerationX);
	//printTextScreen(10 ,30 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "Y acceleration: %f", MainCharacter.AccelerationY);
	//printTextScreen(10 ,40 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "InAir: %d", MainCharacter.InAir);
	//printTextScreen(10 ,50 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "BulletX: %d", BulletX);
	//printTextScreen(10 ,60 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "BulletY: %d", BulletY);
	//printTextScreen(10 ,70 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "TempX: %d", TempX);
	//printTextScreen(10 ,80 , TempString, 0xFFFFFFFF);
	
	//sprintf(TempString, "TempY: %d", TempY);
	//printTextScreen(10 ,90 , TempString, 0xFFFFFFFF);
	
	//Check if level was beaten
	if(LevelOver == true) CurrentScene = SceneTileset;
	
	//Check if game ended (restart level)
	if(GameOver == true) CurrentScene = SceneTileset;
}

//Level editor scene
void EditorFunction(void)
{
	int X = 0;
	int Y = 0;
	
	//Reset game status
	GameOver = false;
	LevelOver = false;
	
	if(GamePad.Buttons & PSP_CTRL_SELECT && SelectFlag == false)
	{
		CurrentScene = SceneSetMenu;
		SelectFlag = true;
	}
	
	//Reset select flag if select is not pressed
	if((GamePad.Buttons & PSP_CTRL_SELECT) == false) SelectFlag = false;
	
	if(GamePad.Buttons & PSP_CTRL_UP && DPADTimeout == 0)
	{
		if(EditorSelectorY > 0) EditorSelectorY--;
		DPADTimeout = 5;
	}
	
	if(GamePad.Buttons & PSP_CTRL_DOWN && DPADTimeout == 0)
	{
		if(EditorSelectorY < 16) EditorSelectorY++;
		DPADTimeout = 5;
	}
		
	if(GamePad.Buttons & PSP_CTRL_LEFT && DPADTimeout == 0)
	{
		if(EditorSelectorX > 0) EditorSelectorX--;
		DPADTimeout = 5;
	}
	
	if(GamePad.Buttons & PSP_CTRL_RIGHT && DPADTimeout == 0)
	{
		if(EditorSelectorX < 29) EditorSelectorX++;
		DPADTimeout = 5;
	}
	
	if(GamePad.Buttons & PSP_CTRL_LTRIGGER && ButtonsTimeout == 0)
	{
		if(EditorSelectedItem > 0) EditorSelectedItem--;
		ButtonsTimeout = 10;
	}
	
	if(GamePad.Buttons & PSP_CTRL_RTRIGGER && ButtonsTimeout == 0)
	{
		if(EditorSelectedItem < 6) EditorSelectedItem++;
		ButtonsTimeout = 10;
	}
	
	if(GamePad.Buttons & PSP_CTRL_SQUARE)
	{
		EditorPlayfield[EditorSelectorX + (EditorSelectorY * 30)] = EditorSelectedItem;
	}
	
	//Play test the created level
	if(GamePad.Buttons & PSP_CTRL_START)
	{
		CurrentScene = SceneTileset;
	}
	
	//Decrease DPAD timeout
	if(DPADTimeout > 0)DPADTimeout--;
	if(ButtonsTimeout > 0)ButtonsTimeout--;
	
	//Draw background
	blitImageToImage(0 ,0 ,480 , 272, BackgroundImage, 0, 0, EditorFramebuffer);
	
	//Draw tileset on screen
	for(Y = 0; Y < 17; Y++)
	{
		for(X = 0; X < 30; X++)
		{
			switch(EditorPlayfield[Y*30 + X])
			{
				default:
				case TlsBlank:
					//Don't draw anything
					break;
					
				case TlsMoon:
					blitAlphaImageToImage(16, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
					
				case TlsMetal:
					blitAlphaImageToImage(32, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
					
				case TlsLava:
					blitAlphaImageToImage(48, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
					
				case TlsLauncher:
					blitAlphaImageToImage(64, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
				
				case TlsStart:
					blitAlphaImageToImage(80, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
					
				case TlsEnd:
					blitAlphaImageToImage(96, 0, 16, 16, TilesetImage, X*16, Y*16, EditorFramebuffer);
					break;
			}
		}
	}
	
	//Draw framebuffer
	blitImageToScreen(0 ,0 ,480 , 272, EditorFramebuffer, 0, 0);
	
	//Draw selected item
	blitAlphaImageToScreen(EditorSelectedItem * 16, 0, 16 , 16, TilesetImage, EditorSelectorX * 16, EditorSelectorY * 16);
		
	//Draw 16x16 selector
	blitAlphaImageToScreen(64 ,16 ,16 , 16, TilesetImage, EditorSelectorX * 16, EditorSelectorY * 16);
}

//Calculate path for the bullet
void CalculateTrajectory(void)
{
	int i = 0;
	float X = 0;
	float Y = 0;
	float DeltaX = 0;
	float DeltaY = 0;
	float FinalX = 0;
	float FinalY = 0;
	
	X = PortalPointer.X - MainCharacter.X;
	Y = PortalPointer.Y - MainCharacter.Y;
	DeltaX = X / 8160;
	DeltaY = Y / 8160;
	FinalX = MainCharacter.X + 8;
	FinalY = MainCharacter.Y + 8;
	
	for(i = 0; i < 8160; i++)
	{
		if(getPixelImage(FinalX, FinalY, TilesetFramebuffer) != 0) break;
		FinalX += DeltaX;
		FinalY += DeltaY;
	}
	
	BulletX = FinalX;
	BulletY = FinalY;
}

//Set default values
void SetDefaults(void)
{
	MainCharacter.X = 0;
	MainCharacter.Y = 0;
	MainCharacter.AccelerationX = 0;
	MainCharacter.AccelerationY = 0;
	MainCharacter.InAir = true;
	MainCharacter.Steps = 0;
	MainCharacter.Direction = right;
	
	OrangePortal.X = 0;
	OrangePortal.Y = 0;
	OrangePortal.Rotation = left;
	OrangePortal.Visible = false;
	
	BluePortal.X = 16;
	BluePortal.Y = 16;
	BluePortal.Rotation = left;
	BluePortal.Visible = false;
	
	PortalPointer.X = 0;
	PortalPointer.Y = 0;
	PortalPointer.Frame = 0;
	PortalPointer.Degree = 0;
	PortalPointer.LineVisible = false;
	
	OrangePortalDelay = 0;
	BluePortalDelay = 0;
	
	LaunchPad.X = 0;
	LaunchPad.Y = 0;
	LaunchPad.Rotation = 0;		//Unused
	LaunchPad.Visible = false;
	
	EndPoint.X = 30;
	EndPoint.Y = 30;
	EndPoint.Rotation = 0;		//Unused
	EndPoint.Visible = false;
	
	ShootDelay = 0;
	
	DPADTimeout = 0;
	ButtonsTimeout = 0;
}

//Load default settings for level editor
void SetEditorDefaults(void)
{
	int i = 0;
	
	freeImage(BackgroundImage);
	BackgroundImage = loadImage("images/background_1.png");
	
	//Clear editor framebuffer
	clearImage(0x00000000, EditorFramebuffer);
	
	EditorSelectorX = 0;
	EditorSelectorY = 0;
	EditorSelectedItem = 0;
	
	DPADTimeout = 0;
	ButtonsTimeout = 0;
	
	//Clear playfield
	for(i = 0; i< 511; i++)
	{
		EditorPlayfield[i] = 0;
	}
}

//Copy sample level to current level playfield
void CopySampleToCurrent(int SampleIndex)
{
	int i = 0;
	
	for(i = 0; i < 511; i++)
	{
		GamePlayfield[i] = SampleLevels[(511*SampleIndex) + i];
	}
}

//Copy edited level to current level playfield
void CopyEditedToCurrent()
{
	int i = 0;
	
	for(i = 0; i < 511; i++)
	{
		GamePlayfield[i] = EditorPlayfield[i];
	}
}

//Exit callback
int exit_callback(int arg1, int arg2, void *common) {
          sceKernelExitGame();
          return 0;
}

//Callback thread
int CallbackThread(SceSize args, void *argp) {
          int cbid;

          cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
          sceKernelRegisterExitCallback(cbid);

          sceKernelSleepThreadCB();

          return 0;
}

//Sets up the callback thread and returns its thread id
int SetupCallbacks(void) {
          int thid = 0;

          thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
          if(thid >= 0) {
                    sceKernelStartThread(thid, 0, 0);
          }

          return thid;
}
