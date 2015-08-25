// Copyright (C) 2009 Luis and Zach Thibeau from http://emucraze.com

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <windows.h>
#include <d3dx9.h>
#include "ioregs.h"
#include "res.h"

extern LPDIRECT3DDEVICE9 lpd3ddev; extern DWORD DISP3DCNT;
extern BOOL bSwap; extern LPDIRECT3DTEXTURE9 lpd3dtex;
D3DPRIMITIVETYPE d3dpt; LPDIRECT3DVERTEXDECLARATION9 lpd3dvd;
D3DTRANSFORMSTATETYPE mtxmode;

void I_CDECL ioALPHA_TEST_REF( u32 addr, u32 data, u8 am )
{
	lpd3ddev->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
	lpd3ddev->SetRenderState( D3DRS_ALPHAREF, data&0xFF );
	wsprintf( sz, _T("ioALPHA_TEST_REF: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioBEGIN_VTXS( u32 addr, u32 data, u8 am )
{
	switch (data&0x3)
	{
	case 0:
		d3dpt = D3DPT_TRIANGLELIST;
		D3DVERTEXELEMENT9 d3dve;
		ZeroMemory( &d3dve, sizeof(D3DVERTEXELEMENT9) );
		lpd3ddev->CreateVertexDeclaration(&d3dve, &lpd3dvd);
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
	wsprintf( sz, _T("ioBEGIN_VTXS: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioEND_VTXS( u32 addr, u32 data, u8 am )
{
}

void I_CDECL ioVTX( u32 addr, u32 data, u8 am )
{
}

void I_CDECL ioDISP3DCNT( u32 addr, u32 data, u8 am )
{
	DWORD tmpDISPCNT = data;
	lpd3ddev->SetRenderState( D3DRS_ALPHATESTENABLE, data&0x2 );
	lpd3ddev->SetRenderState( D3DRS_ALPHABLENDENABLE, data&0x3 );
	lpd3ddev->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, data&0x4 );
	lpd3ddev->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, data&0x5 );
	lpd3ddev->SetRenderState( D3DRS_FOGENABLE, data&0x7 );
	data = DISP3DCNT;
	DISP3DCNT = tmpDISPCNT;
	wsprintf( sz, _T("ioDISP3DCNT: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioMTX_MODE( u32 addr, u32 data, u8 am )
{
	switch (data&0x3)
	{
	case 0:
		mtxmode = D3DTS_PROJECTION;
		break;
	case 1:
		mtxmode = D3DTS_VIEW;
		break;
	case 3:
		mtxmode = D3DTS_TEXTURE0;
		break;
	default:
		// Not Supported yet or invalid mode
		break;
	}
	wsprintf( sz, _T("ioMTX_MODE: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioMTX_IDENTITY( u32 addr, u32 data, u8 am )
{
	D3DXMATRIX mtx;
	lpd3ddev->SetTransform(mtxmode, D3DXMatrixIdentity(&mtx));
	wsprintf( sz, _T("ioMTX_IDENTITY: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioSWAPBUFFERS( u32 addr, u32 data, u8 am )
{
	lpd3ddev->SetRenderState( D3DRS_ZENABLE, data&0x2?D3DZB_USEW:D3DZB_TRUE );
	lpd3ddev->Present(NULL, NULL, NULL, NULL);
	bSwap = TRUE;
}

void I_CDECL ioTEXIMAGE_PARAM( u32 addr, u32 data, u8 am )
{
	if( ((data>>26)&0x7) == 7 ) {
		D3DLOCKED_RECT d3dlr;
		if( lpd3dtex == NULL )
			D3DXCreateTexture( lpd3ddev, 8<<((data>>20)&0x7), 8<<((data>>23)&0x7),
				D3DX_DEFAULT, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &lpd3dtex );
		lpd3dtex->LockRect( 0, &d3dlr, NULL, D3DLOCK_DISCARD );
		CopyMemory( d3dlr.pBits, (void**)(0x4000240+((data&0xFF)/8)), (8<<((data>>20)&7))+(8<<((data>>23)&7))+sizeof(WORD) );
		lpd3dtex->UnlockRect( 0 );
		//lpd3ddev->UpdateTexture( (LPDIRECT3DTEXTURE9)(data&0xFF), lpd3dtex );
		wsprintf( sz, _T("../%s_%i"), _T("tex"), rand()%0xFFFF );
		D3DXSaveTextureToFile( sz, D3DXIFF_PNG, lpd3dtex, NULL );
	}
	wsprintf( sz, _T("ioTEXIMAGE_PARAM: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}

void I_CDECL ioVIEWPORT( u32 addr, u32 data, u8 am )
{
	D3DVIEWPORT9 d3dvp;
	d3dvp.X = data&0xFF;
	d3dvp.Y = (data>>8)&0xFF;
	d3dvp.Width = (data>>16)&0xFF;
	d3dvp.Height = (data>>24)&0xFF;
	d3dvp.MinZ = 0.0f;
	d3dvp.MaxZ = 1.0f;
	lpd3ddev->SetViewport( &d3dvp );

	wsprintf( sz, _T("ioVIEWPORT: %X,%X,%X\n"), addr, data, am );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
}
