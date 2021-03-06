// HOOK.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

//变量声明
int* d3d_Dw5byte = new int;//DrawIndexedPrimitive入口的前5个字节
int* d3d_Es5byte = new int;//EndScence入口的前5个字节
ULONG_PTR g_DwJmpTo = NULL;//DrawIndexedPrimitive调用地址
ULONG_PTR g_DwAdder = NULL;//DrawIndexedPrimitive被HOOK后地址
ULONG_PTR g_EsJmpTo = NULL;//EndScence调用地址
ULONG_PTR g_EsAdder = NULL;//EndScence被HOOK后地址
BOOL g_FontCreated = FALSE;//是否创建字体
static LPD3DXFONT g_Font = NULL;//D3D字体对象
IDirect3DPixelShader9* Front = NULL;
IDirect3DPixelShader9* Back = NULL;
IDirect3DTexture9* Color_TexRed;//红色纹理
IDirect3DTexture9* Color_TexGreen;//绿色纹理



//函数声明
HRESULT WINAPI hk_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 Device_Interface, D3DPRIMITIVETYPE Type, INT BaseIndex, UINT MinIndex, UINT NumVertices, UINT AAAAAAAA, UINT PrimitiveCount);
HRESULT WINAPI hk_EndScence(LPDIRECT3DDEVICE9 m_pDevice);
HRESULT WINAPI Original_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 m_pDevice, D3DPRIMITIVETYPE type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
HRESULT WINAPI Original_EndScence(LPDIRECT3DDEVICE9 m_pDevice);
ULONG_PTR GetDrawIndexedPrimitiveAdder();
ULONG_PTR GetEndScenceAdder();
void OnHookInit();
void DrawFont(int X, int Y, D3DCOLOR Color, char *format, ...);


/*函数定义*/

//导出函数
void demo() {
	OnHookInit();
}
//初始化HOOK
void OnHookInit() {
	MessageBox(NULL, L"注入成功！", L"提示:", NULL);
	g_DwAdder = GetDrawIndexedPrimitiveAdder();
	g_EsAdder = GetEndScenceAdder();
	g_DwJmpTo = g_DwAdder + 5;
	g_EsJmpTo = g_EsAdder + 7;
	DWORD oldpro = 0;


	memcpy(d3d_Dw5byte, (VOID*)g_DwAdder, 5);//保存DrawIndexedPrimitive入口的前5个字节
	VirtualProtect((LPVOID)g_DwAdder, 5, PAGE_EXECUTE_READWRITE, &oldpro);//修改内存保护属性
	*(BYTE*)g_DwAdder = 0xe9;//0xe9在汇编中是跳转指令操作码  
	*(DWORD*)((BYTE*)g_DwAdder + 1) = (DWORD)hk_DrawIndexedPrimitive - (DWORD)g_DwAdder - 5;//目标地址-原地址-5  
	VirtualProtect((LPVOID)g_DwAdder, 5, oldpro, &oldpro);//还原内存保护属性

	memcpy(d3d_Es5byte, (VOID*)g_EsAdder, 7);//保存EndScence入口的前7个字节
	VirtualProtect((LPVOID)g_EsAdder, 7, PAGE_EXECUTE_READWRITE, &oldpro);//修改内存保护属性
	*(BYTE*)g_EsAdder = 0x90;//占用两个字节防止出现错误
	*(BYTE*)(g_EsAdder + 1) = 0x90;//占用两个字节防止出现错误
	*(BYTE*)(g_EsAdder + 2) = 0xe9;//0xe9在汇编中是跳转指令操作码  
	*(DWORD*)((BYTE*)g_EsAdder + 3) = (DWORD)hk_EndScence - (DWORD)g_EsAdder - 7;//目标地址-原地址-7
	VirtualProtect((LPVOID)g_EsAdder, 7, oldpro, &oldpro);//还原内存保护属性
}
//描绘字体
void DrawFont(int X,int Y,D3DCOLOR Color,char *format,...) {
	char Buffer[256];
	va_list str;
	va_start(str,format);
	vsprintf(Buffer,format,str);
	RECT FontRect = { X, Y, X + 120, Y + 16 };
	g_Font->DrawTextA(NULL, Buffer, -1, &FontRect, DT_NOCLIP, Color);
	va_end (str);
}
//获取DrawIndexedPrimitive地址
ULONG_PTR GetDrawIndexedPrimitiveAdder() {
	HMODULE m_hModule = GetModuleHandle(L"d3d9.dll");
	return (ULONG_PTR)m_hModule + 0x57740;
}
//获取EndScence地址
ULONG_PTR GetEndScenceAdder() {
	HMODULE m_hModule = GetModuleHandle(L"d3d9.dll");
	return (ULONG_PTR)m_hModule + 0x56DD0;
}
//调用真实的DrawIndexedPrimitive函数
__declspec(naked) HRESULT WINAPI Original_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 m_pDevice, D3DPRIMITIVETYPE type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	__asm
	{
		mov edi, edi
		push ebp
		mov ebp, esp
		mov eax, g_DwJmpTo
		jmp eax
	}
}
//调用真实的EndScence函数
__declspec(naked) HRESULT WINAPI Original_EndScence(LPDIRECT3DDEVICE9 m_pDevice)
{
	__asm
	{
		push 18h
		mov eax, 61DB8975h
		mov eax, g_EsJmpTo
		jmp eax
	}
}
//创建纹理
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev,IDirect3DTexture9 **ppD3Dtex,DWORD colour32)
{
	if (FAILED(pD3Ddev->CreateTexture(8, 8, 1, 0,
		D3DFMT_A4R4G4B4,
		D3DPOOL_MANAGED,
		ppD3Dtex, NULL)))
		return E_FAIL;
	WORD colour16 =
		((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);
	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;
	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;
	(*ppD3Dtex)->UnlockRect(0);
	return S_OK;
};
//DrawIndexedPrimitive被HOOK的函数
HRESULT WINAPI hk_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	IDirect3DVertexBuffer9* pStreamData =	NULL;
	UINT iOffsetInBytes;
	UINT iStride;	
	if (pDevice->GetStreamSource(0, &pStreamData, &iOffsetInBytes, &iStride) == D3D_OK)
	{
		pStreamData->Release();
	}
	char str[512];
	sprintf(str,"现在的Stride数据是:%d", iStride);
	OutputDebugStringA(str);
	//pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);//禁用z轴缓冲
	//result = Original_DrawIndexedPrimitive(pDevice,type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	if (iStride == 72)//设置iStride为72
	{
		DWORD pValue;
		pDevice->SetPixelShader(Front);//清空着色器
		pDevice->SetTexture(0, Color_TexRed);//上色
		pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);//禁用z轴缓冲
		//pDevice->GetRenderState(D3DRS_ZENABLE,&pValue);//保存缓存状态
		pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_NEVER);
		Original_DrawIndexedPrimitive(pDevice, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		//pDevice->SetRenderState(D3DRS_ZENABLE, pValue);
		pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);//线填充模式，D3D在多边形的每个边绘制一条线
	}
	else
	{
		pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);//恢复禁用轴缓冲
		//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);//恢复默认线条
	}
	return Original_DrawIndexedPrimitive(pDevice, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}
//EndScence被HOOK的函数
HRESULT WINAPI hk_EndScence(LPDIRECT3DDEVICE9 pDevice) {
	GenerateTexture(pDevice, &Color_TexRed, D3DCOLOR_ARGB(255, 255, 0, 0));//上色背景 红
	GenerateTexture(pDevice, &Color_TexGreen, D3DCOLOR_ARGB(255, 0, 255, 0));//上色背景 绿 


	if (!g_FontCreated)//防止二次重复创建字体
	{
		D3DXCreateFont(pDevice, 12, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEVICE_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Visitor TT2 (BRK)", &g_Font);
		g_FontCreated = TRUE;//字体创建成功
	}
	DrawFont(3, 10, D3DCOLOR_ARGB(255, 238, 17, 216), "终极火力HOOK透视辅助");

	return Original_EndScence(pDevice);
}