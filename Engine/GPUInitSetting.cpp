#include "PCH_Engine.h"
#include "GPUInitSetting.h"
#include "StructBuffer.h"
#include "DefaultShader/NormalConverter.hlsli"
#include "DefaultShaders.h"

namespace ehw
{
	GPUInitSetting::GPUInitSetting()
		: ComputeShader(uint3(1u, 1u, 1u))
	{
		SetEngineDefaultRes(true);
	}
	GPUInitSetting::~GPUInitSetting()
	{
	}

	eResult GPUInitSetting::Load(const std::filesystem::path& _filePath)
	{
		IResource::Load(_filePath);

		eResult result = CreateByHeader(CS_GPUInitSetting, sizeof(CS_GPUInitSetting));

		ASSERT(eResultSuccess(result), "GPU 초기화 작업 실패");


		return result;
	}

	bool GPUInitSetting::BindData()
	{
		union CheckEndianness
		{
			UINT32	u32;
			INT8	i8[4];
		};

		{
			CheckEndianness check{};
			check.u32 = (UINT32)1;
			if (check.i8[0])
			{
				//BOOL(1)을 넣어줬을 경우 둘 사이의 endian값이 다르면 byte order이 달라지므로 1이 아닌 0xffffffff를 사용한다
				gGPUInitSetting.bCPULittleEndian = 0xffffffff;
			}
		}

		//리틀 엔디안일 경우 앞 쪽에 1을 넣는다.
		if (gGPUInitSetting.bCPULittleEndian)
		{
			CheckEndianness check{};
			check.i8[0] = (UINT8)1;
			gGPUInitSetting.GPUEndianessTestVar = check.u32;
		}

		//빅 엔디안일 경우 뒷 쪽에 1을 넣는다
		else
		{
			CheckEndianness check{};
			check.i8[3] = (UINT8)1;
			gGPUInitSetting.GPUEndianessTestVar = check.u32;
		}

		gGPUInitSetting.debugVar.x = 0x12345678;

		//데이터 전송
		//SBuffer
		StructBuffer::Desc desc{};
		desc.eSBufferType = eStructBufferType::READ_WRITE;
		desc.REGISLOT_t_SRV = Register_t_gInitSetting;
		desc.REGISLOT_u_UAV = Register_u_gInitSettingRW;
		mInitSBuffer = std::make_unique<StructBuffer>(desc);

		HRESULT result = mInitSBuffer->Create<tGPUInitSetting>(1ui64, &gGPUInitSetting, 1ui64);
		ASSERT(SUCCEEDED(result), "GPU 초기화용 구조화 버퍼 생성 실패.");

		mInitSBuffer->BindDataUAV();

		ComputeShader::CalculateGroupCount(uint3(1u, 1u, 1u));

		return true;
	}

	void GPUInitSetting::UnBindData()
	{
		mInitSBuffer->UnBindData();
		mInitSBuffer->GetData(&gGPUInitSetting);
		mInitSBuffer->BindDataSRV();
	}
}
