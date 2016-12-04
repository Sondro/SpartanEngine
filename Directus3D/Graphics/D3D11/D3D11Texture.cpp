/*
Copyright(c) 2016 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =================
#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"
#include "../../Logging/Log.h"
//============================

//= NAMESPAECES ====
using namespace std;
//==================

D3D11Texture::D3D11Texture(D3D11GraphicsDevice* graphics)
{
	m_shaderResourceView = nullptr;
	m_graphics = graphics;

	m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_mipLevels = 1;
}


D3D11Texture::~D3D11Texture()
{
	if (m_shaderResourceView)
	{
		m_shaderResourceView->Release();
		m_shaderResourceView = nullptr;
	}
}

// Creates a texture with no mipchain
bool D3D11Texture::Create(int width, int height, int channels, unsigned char* data)
{
	m_mipLevels = 1;

	//= SUBRESROUCE DATA =======================================================================
	D3D11_SUBRESOURCE_DATA subresource;
	ZeroMemory(&subresource, sizeof(subresource));
	subresource.pSysMem = data;
	subresource.SysMemPitch = (width * channels) * sizeof(unsigned char);
	subresource.SysMemSlicePitch = (width * height * channels) * sizeof(unsigned char);
	//==========================================================================================

	//= ID3D11Texture2D ========================================================================
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = (UINT)width;
	textureDesc.Height = (UINT)height;
	textureDesc.MipLevels = m_mipLevels;
	textureDesc.ArraySize = (UINT)1;
	textureDesc.Format = m_format;
	textureDesc.SampleDesc.Count = (UINT)1;
	textureDesc.SampleDesc.Quality = (UINT)0;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;
	textureDesc.CPUAccessFlags = 0;

	// Create texture from description
	ID3D11Texture2D* texture = nullptr;
	HRESULT result = m_graphics->GetDevice()->CreateTexture2D(&textureDesc, &subresource, &texture);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
		return false;
	}
	//=========================================================================================

	//= SHADER RESOURCE VIEW ==================================================================
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_mipLevels;

	// Create shader resource view from description
	result = m_graphics->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create the ID3D11ShaderResourceView.");
		return false;
	}
	//========================================================================================

	return true;
}

// Creates a texture and generates a mipchain for it. Good for when you need a mipchain with 
// no extra trouble, not so fast as the texture generated by a predefined mipchain though.
bool D3D11Texture::CreateAndGenerateMipchain(int width, int height, int channels, unsigned char* data)
{
	m_mipLevels = 7;

	//= ID3D11Texture2D ========================================================================
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = (UINT)width;
	textureDesc.Height = (UINT)height;
	textureDesc.MipLevels = m_mipLevels;
	textureDesc.ArraySize = (UINT)1;
	textureDesc.Format = m_format;
	textureDesc.SampleDesc.Count = (UINT)1;
	textureDesc.SampleDesc.Quality = (UINT)0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	textureDesc.CPUAccessFlags = 0;

	// Create texture from description
	ID3D11Texture2D* texture = nullptr;
	HRESULT result = m_graphics->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &texture);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
		return false;
	}
	//=========================================================================================

	//= SHADER RESOURCE VIEW ==================================================================
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_mipLevels;

	// Create shader resource view from description
	result = m_graphics->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create the ID3D11ShaderResourceView.");
		return false;
	}
	//========================================================================================

	//= MIPCHAIN GENERATION ==================================================================
	// Resource data description
	D3D11_SUBRESOURCE_DATA subresource;
	ZeroMemory(&subresource, sizeof(subresource));
	subresource.pSysMem = data;
	subresource.SysMemPitch = (width * channels) * sizeof(unsigned char);
	subresource.SysMemSlicePitch = (width * height * channels) * sizeof(unsigned char);

	// Copy data from memory to the subresource created in non-mappable memory
	m_graphics->GetDeviceContext()->UpdateSubresource(texture, 0, nullptr, subresource.pSysMem, subresource.SysMemPitch, 0);

	// Create mipchain based on ID3D11ShaderResourveView
	m_graphics->GetDeviceContext()->GenerateMips(m_shaderResourceView);
	//========================================================================================

	return true;
}

// Creates a texture from a predefined mipchain. This is the fastest performing type of texture that also includes a mipchain.
bool D3D11Texture::CreateFromMipchain(int width, int height, int channels, const vector<vector<unsigned char>>& mipchain)
{
	m_mipLevels = (UINT)mipchain.size();

	//= SUBRESOURCE DATA & TEXTURE DESCRIPTIONS ==============================================
	vector<D3D11_SUBRESOURCE_DATA> subresourceData;
	vector<D3D11_TEXTURE2D_DESC> textureDescs;
	for (const auto& mipLevel : mipchain)
	{
		// Create subresource
		subresourceData.push_back(D3D11_SUBRESOURCE_DATA{});
		subresourceData.back().pSysMem = mipLevel.data();
		subresourceData.back().SysMemPitch = (width * channels) * sizeof(unsigned char);
		subresourceData.back().SysMemSlicePitch = (width * height * channels) * sizeof(unsigned char);

		textureDescs.push_back(D3D11_TEXTURE2D_DESC{});
		textureDescs.back().Width = (UINT)width;
		textureDescs.back().Height = (UINT)height;
		textureDescs.back().MipLevels = m_mipLevels;
		textureDescs.back().ArraySize = (UINT)1;
		textureDescs.back().Format = m_format;
		textureDescs.back().SampleDesc.Count = (UINT)1;
		textureDescs.back().SampleDesc.Quality = (UINT)0;
		textureDescs.back().Usage = D3D11_USAGE_IMMUTABLE;
		textureDescs.back().BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDescs.back().MiscFlags = 0;
		textureDescs.back().CPUAccessFlags = 0;

		width = max(width / 2, 1);
		height = max(height / 2, 1);
	}
	//==========================================================================================

	//= ID3D11Texture2D ========================================================================
	// Create texture from description
	ID3D11Texture2D* texture = nullptr;
	HRESULT result = m_graphics->GetDevice()->CreateTexture2D(&textureDescs[0], &subresourceData[0], &texture);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
		return false;
	}
	//=========================================================================================

	//= SHADER RESOURCE VIEW ==================================================================
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_mipLevels;

	// Create shader resource view from description
	result = m_graphics->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
	if (FAILED(result))
	{
		LOG_ERROR("Failed to create the ID3D11ShaderResourceView.");
		return false;
	}
	//========================================================================================

	return true;
}