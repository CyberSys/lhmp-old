/**
	Lost Heaven Multiplayer

	Purpose: ingame menu (after ESC) with news from our web site

	@author Romop5
	@version 1.0 1/9/14
	@todo clean code / check news width before render
*/

#ifndef __CINGAMEMENU_H
#define __CINGAMEMENU_H

#include <d3d8.h>
#include <d3dx8.h>
class CIngameMenu
{
private:

	bool	m_bIsActive;
	LPD3DXFONT	ahojmoj;
	LPD3DXFONT	menuitem;
	LPD3DXFONT	clock;

	int		itemSelect;

	LPDIRECT3DTEXTURE8 MenuTexture;
public:
	CIngameMenu();
	void	Init();
	void	Render();
	void	Tick();
	void	OnArrowUp();
	void	OnArrowDown();
	void	OnPressEnter();

	bool	isActive();
	void	setActive(bool);

	void	OnLostDevice();
	void	OnResetDevice();

	char*	ZeroFormat(int,char*);
	char newsBuff[1024];
};

#endif