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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <windows.h>
#include <d3dx9.h>
#include "ioregs.h"
#include "guid.h"
#include "res.h"

TCHAR sz[256];
HANDLE hCon;
DWORD dw;

HINSTANCE hInst; HWND hTop, hBottom;
LPDIRECT3D9 lpd3d; LPDIRECT3DDEVICE9 lpd3ddev;
LPDIRECT3DSWAPCHAIN9 lpTopLcd, lpBottomLcd;
LPDIRECT3DTEXTURE9 lpd3dtex; LPDIRECT3DSURFACE9 lpd3dsurf;
LPDIRECT3DVERTEXBUFFER9 lpd3dvb;
D3DLOCKED_RECT d3dlr; HDC hdc;
DWORD outbuf[256*192], POWCNT, DISP3DCNT; BOOL bSwap;
IPlugInManager* pim; IPlugInInterface* pii; VIDEO3DPARAM v3dp;

extern "C" DWORD __declspec(dllexport) I_STDCALL ResetFunc();
void Init3DTable();

extern "C" BOOL APIENTRY DllMain( HINSTANCE hInst, DWORD reason, LPVOID reserved )
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			::hInst = hInst;
			#ifdef _DEBUG
				hCon = GetStdHandle(STD_OUTPUT_HANDLE);
				if( !hCon ) {
					AllocConsole();
					hCon = GetStdHandle(STD_OUTPUT_HANDLE);
				}
			#endif
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL GetInfoFunc( LPGETPLUGININFO p )
{
	p->dwType = PIT_VIDEO | PIT_DYNAMIC | PIT_IS3D | PIT_NOMODIFY;
	if( p->pszText != NULL ) {
		switch (LOWORD(p->dwLanguageID)) {
			case LANG_ENGLISH:
			default:
				lstrcpy(p->pszText, (LPTSTR)NAME_PLUGIN);
				break;
		}
	}
	*(&p->guidID) = *(&D3DGUID);
	wsprintf( sz, _T("Get: %d,%d,%d,%d\n"), p->wIndex, p->wType, p->dwLanguageID, p->lParam );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
	return 1;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL SetInfoFunc( LPSETPLUGININFO p )
{
	if( p == NULL )
		return FALSE;
	if( PLUGINISENABLE(p) ) {
		IWnd* lcd;
		v3dp = *((VIDEO3DPARAM*)p->lParam);
		p->lpNDS->QueryInterface( IID_UPLCD, (void**)&lcd );
		hTop = lcd->Handle();
		p->lpNDS->QueryInterface( IID_DOWNLCD, (void**)&lcd );
		hBottom = lcd->Handle();
		p->lpNDS->QueryInterface( IID_IPLUGINMANAGER, (void**)&pim);
		pim->get_PlugInInterface( &D3DGUID, (void**)&pii );
		if( !lpd3d ) {
			lpd3d = Direct3DCreate9(D3D_SDK_VERSION);
		}
		if( !lpd3ddev ) {
			D3DPRESENT_PARAMETERS d3dpp; D3DCAPS9 d3dc;

			lpd3d->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dc );
			wsprintf( sz, _T("PS Version:%d.%d, VS Version:%d.%d, %d\n"),
				D3DSHADER_VERSION_MAJOR(d3dc.PixelShaderVersion),
				D3DSHADER_VERSION_MINOR(d3dc.PixelShaderVersion),
				D3DSHADER_VERSION_MAJOR(d3dc.VertexShaderVersion),
				D3DSHADER_VERSION_MINOR(d3dc.VertexShaderVersion),
				d3dc.NumSimultaneousRTs );
			WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );

			ZeroMemory( &d3dpp, sizeof(D3DPRESENT_PARAMETERS) );
			d3dpp.Windowed = TRUE;
			d3dpp.hDeviceWindow = hTop;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
			d3dpp.EnableAutoDepthStencil = TRUE;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;

			if( FAILED(lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hTop,
				D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpd3ddev)) )
				if( FAILED(lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hTop,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3ddev)) ) {
					lpd3d->Release();
					return FALSE;
				}
			lpd3ddev->GetSwapChain( 0, &lpTopLcd );
			d3dpp.hDeviceWindow = hBottom;
			lpd3ddev->CreateAdditionalSwapChain( &d3dpp, &lpBottomLcd );
		}
		ResetFunc();
		Init3DTable();
		/*p->lpNDS->QueryInterface(IID_IPLUGINMANAGER, (void**)&pim);
		pim->get_PlugInInterface(&D3DGUID, (void**)&pii);
		pii->get_Object(OID_VRAM_MEMORY, (void**)&v3dp.video_mem);
		pii->get_Object(OID_IO_MEMORY9, (void**)&v3dp.io_mem);*/
		wsprintf( sz, _T("D3D is enabled\n") );
		WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
	} else if( PLUGINISRUN(p) ) {
		wsprintf( sz, _T("D3D is running\n") );
		WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
		lpd3ddev->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, TRUE );
	}
	if( p->dwStateMask != PIS_NOTIFYMASK )
		return TRUE;
	else if( p->dwStateMask == PIS_NOTIFYMASK )
		switch (p->dwState) {
			case PNM_STARTFRAME:
				/*lpd3ddev->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
					D3DCOLOR_XRGB(255,0,0), 1.0f, 0 );*/
				lpd3ddev->BeginScene();
				wsprintf( sz, _T("BeginFrame\n") );
				WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
				return TRUE;
			case PNM_ENDFRAME:
				lpd3ddev->EndScene();
				wsprintf( sz, _T("EndFrame\n") );
				WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
				return TRUE;
			case PNM_ENTERVBLANK:
				if( !bSwap )
					return TRUE;
				bSwap = FALSE;
				if( POWCNT & 0x80 )
					lpTopLcd->Present( NULL, NULL, NULL, NULL, 0 );
				else
					lpBottomLcd->Present( NULL, NULL, NULL, NULL, 0 );
				wsprintf( sz, _T("RenderFrame\n") );
				WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
				break;
			case PNM_ENDLINE:
				return TRUE;
			case PNM_COUNTFRAMES:
			case PNM_ONACTIVATEWINDOW:
				return TRUE;
			case PNM_OPENFILE:
				wsprintf( sz, _T("%s\n"), (LPTSTR)p->lParam );
				WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
			case PNMV_OAMCHANGED:
			case PNMV_PALETTECHANGED:
				return TRUE;
			case PNMV_CHANGEVRAMCNT: {
				if( LOWORD(p->lParam) < 4 ) {
					switch (HIWORD(p->lParam)) {
						case 0x83:
						case 0x8B:
						case 0x93:
						case 0x9B:
							break;
					}
				}
				wsprintf( sz, _T("%X,%X\n"), LOWORD(p->lParam), HIWORD(p->lParam) );
				WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
			}	break;
			case PNMV_GETBUFFER: {
				*((u32**)p->lParam) = outbuf;
			}	return TRUE;
			case PNM_POWCNT1:
				POWCNT = p->lParam;
				if( lpd3dsurf )
					lpd3dsurf->Release();
				if( POWCNT & 0x80 )
					lpTopLcd->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &lpd3dsurf);
				else
					lpBottomLcd->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &lpd3dsurf);
				lpd3ddev->SetRenderTarget( 0, lpd3dsurf );
			default:
				break;
		}
	wsprintf( sz, _T("Set: %d,%X,%X,%X\n"), p->wIndex, p->dwState, p->dwStateMask, p->lParam );
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL ResetFunc()
{
	FillMemory( outbuf, 256*192*4, 0xFF );
	if( lpd3ddev ) {
	lpd3ddev->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	lpd3ddev->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	//lpd3ddev->SetRenderState( D3DRS_STENCILENABLE, TRUE );
	lpd3ddev->SetRenderState( D3DRS_DITHERENABLE, TRUE );
	lpd3ddev->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_LINEAR );
	lpd3ddev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	lpd3ddev->SetRenderState( D3DRS_LIGHTING, FALSE );
	lpd3ddev->LightEnable( 0, FALSE );
	lpd3ddev->LightEnable( 1, FALSE );
	lpd3ddev->LightEnable( 2, FALSE );
	lpd3ddev->LightEnable( 3, FALSE );
	lpd3ddev->SetRenderState( D3DRS_FOGENABLE, FALSE );
	lpd3ddev->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	lpd3ddev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	lpd3ddev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	lpd3ddev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	lpd3ddev->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 0, 0 );
	}
	wsprintf( sz, _T("ResetFunc\n") );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL DeleteFunc()
{
	ResetFunc();
	if( lpd3dvb ) {
		lpd3dvb->Release();
		lpd3dvb = NULL;
	}
	if( lpd3dsurf ) {
		lpd3dsurf->Release();
		lpd3dsurf = NULL;
	}
	if( lpd3dtex ) {
		lpd3dtex->Release();
		lpd3dtex = NULL;
	}
	if( lpBottomLcd ) {
		lpBottomLcd->Release();
		lpBottomLcd = NULL;
	}
	if( lpTopLcd ) {
		lpTopLcd->Release();
		lpTopLcd = NULL;
	}
	if( lpd3ddev ) {
		lpd3ddev->Release();
		lpd3ddev = NULL;
	}
	if( lpd3d ) {
		lpd3d->Release();
		lpd3d = NULL;
	}
	wsprintf( sz, _T("DeleteFunc\n") );
	WriteConsole( hCon, sz, lstrlen(sz), &dw, NULL );
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) I_FASTCALL RunFunc( u32 x, u32 y )
{
	int c, r, g, b;
	c = outbuf[(y*192)+x];
	r = (int)(char)(c>>16); g = (int)(char)(c>>8); b = (int)(char)c;
	return (0x1<<15|(r>>3)<<10|(g>>3)<<5|b);
}

extern "C" DWORD __declspec(dllexport) I_STDCALL SetPropertyFunc( LPSETPROPPLUGIN p )
{
	wsprintf( sz, _T("%d,%d,%d,%d,%d"), p->info.dwType, p->info.wIndex, p->info.wType,
		p->info.dwLanguageID, p->info.lParam );
	MessageBox( (HWND)p->hwndOwner, sz, _T(""), MB_OK );
	return FALSE;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL SaveStateFunc( LStream *p )
{
	wsprintf( sz, _T("%d"), p->Size() );
	MessageBox( hTop, sz, _T(""), MB_OK );
	return TRUE;
}

extern "C" DWORD __declspec(dllexport) I_STDCALL LoadStateFunc( LStream *p, int ver )
{
	wsprintf( sz, _T("%d,%d"), p->Size(), ver );
	MessageBox( hTop, sz, _T(""), MB_OK );
	return TRUE;
}

void Init3DTable()
{
	int i;
	pii->ResetTable();
	for( i = 0x60; i < 0x62; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioDISP3DCNT, NULL );
	pii->WriteTable( 0x4000340, (void*)ioALPHA_TEST_REF, NULL );
	for( i = 0x440; i < 0x444; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioMTX_MODE, NULL );
	for( i = 0x454; i < 0x458; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioMTX_IDENTITY, NULL );
	for( i = 0x4a8; i < 0x4ac; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioTEXIMAGE_PARAM, NULL );
	for( i = 0x500; i < 0x504; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioBEGIN_VTXS, NULL );
	pii->WriteTable( 0x40000504, (void*)ioEND_VTXS, NULL );
	for( i = 0x540; i < 0x544; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioSWAPBUFFERS, NULL );
	for( i = 0x580; i < 0x584; i++ )
		pii->WriteTable( 0x4000000+i, (void*)ioVIEWPORT, NULL );
}
