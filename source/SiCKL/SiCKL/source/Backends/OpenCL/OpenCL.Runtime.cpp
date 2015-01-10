// C
#include <stdio.h>

// local
#include "SiCKL.h"


namespace SiCKL
{
    cl_context OpenCLRuntime::_context = nullptr;
    cl_device_id OpenCLRuntime::_device = nullptr;
    cl_command_queue OpenCLRuntime::_command_queue = nullptr;
    
    namespace Internal
    {
        static size_t TypeSize(ReturnType_t type)
        {
            size_t type_size;
            switch(type)
            {
            case ReturnType::Int:
            case ReturnType::UInt:
            case ReturnType::Float:
                    type_size = 4;
                    break;
            case ReturnType::Int2:
            case ReturnType::UInt2:
            case ReturnType::Float2:
                    type_size = 8;
                    break;
            case ReturnType::Int3:
            case ReturnType::UInt3:
            case ReturnType::Float3:
                    type_size = 12;
                    break;
            case ReturnType::Int4:
            case ReturnType::UInt4:
            case ReturnType::Float4:
                    type_size = 16;
                    break;
            default:
                COMPUTE_ASSERT(false);
            }
            
            return type_size;
        }
    
        static size_t BufferSize(ReturnType::Type type, size_t count)
        {
            return count * TypeSize(type);
        }
    }

    sickl_int OpenCLRuntime::Initialize()
    {
        cl_int err = CL_SUCCESS;
        cl_platform_id platform_id;

        // get our plafrom id
        err = clGetPlatformIDs(1, &platform_id, nullptr);
        if(err == CL_SUCCESS)
        {
            cl_context_properties properties[] =
            {
                CL_CONTEXT_PLATFORM,
                (cl_context_properties)platform_id,
                0,
            };
            // create our context
            _context = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &err);

            size_t device_buffer_size;
            err = clGetContextInfo(OpenCLRuntime::_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_buffer_size);
            if(err == CL_SUCCESS)
            {
                cl_device_id devices[device_buffer_size / sizeof(cl_device_id)];
                err = clGetContextInfo(OpenCLRuntime::_context, CL_CONTEXT_DEVICES, device_buffer_size, devices, nullptr);
                if(err == CL_SUCCESS)
                {
                    _command_queue = clCreateCommandQueue(OpenCLRuntime::_context, devices[0], 0, nullptr);
                    _device = devices[0];
                }
            }
        }
        return err;
    }

    sickl_int OpenCLRuntime::Finalize()
    {
        if(_context != nullptr)
        {
            clReleaseContext(_context);
            clReleaseCommandQueue(_command_queue);
        }
        return SICKL_SUCCESS;
    }

    //
    // OpenCLBuffer1D
    //
    
    OpenCLBuffer1D::OpenCLBuffer1D()
        : Type(ReturnType::Invalid)
        , Length(0)
        , BufferSize(0)
        , _memory_object(nullptr)
    { }

    sickl_int OpenCLBuffer1D::Initialize(size_t length, ReturnType_t type, void* data)
    {
        cl_int err;
        size_t buffer_size = Internal::BufferSize(type, length);

        _memory_object = clCreateBuffer(OpenCLRuntime::_context, CL_MEM_READ_WRITE, buffer_size, data, &err);

        if(err == CL_SUCCESS)
        {
            ReturnType_t* pType = const_cast<ReturnType_t*>(&Type);
            cl_ulong* pLength = const_cast<cl_ulong*>(&Length);
            size_t* pBufferSize = const_cast<size_t*>(&BufferSize);

            *pType = type;
            *pLength = length;
            *pBufferSize = buffer_size;
        }

        return err;
    }

    sickl_int OpenCLBuffer1D::SetData(void* in_buffer)
    {
        return clEnqueueWriteBuffer(OpenCLRuntime::_command_queue, _memory_object, true, 0, BufferSize, in_buffer, 0, nullptr, nullptr);
    }

    sickl_int OpenCLBuffer1D::GetData(void* out_buffer)
    {
        return clEnqueueReadBuffer(OpenCLRuntime::_command_queue, _memory_object, true, 0, BufferSize, out_buffer, 0, nullptr, nullptr);
    }

    void OpenCLBuffer1D::Delete()
    {
        clReleaseMemObject(_memory_object);
        _memory_object = nullptr;
    }
    
    //
    // OpenCLBuffer2D
    //
    
    OpenCLBuffer2D::OpenCLBuffer2D()
        : Type(ReturnType::Invalid)
        , Width(0)
        , Height(0)
        , BufferSize(0)
        , _memory_object(nullptr)
    { }
    
    sickl_int OpenCLBuffer2D::Initialize(size_t width, size_t height, ReturnType_t type, void *data)
    {
        cl_int err;
        size_t buffer_size = Internal::BufferSize(type, width * height);

        _memory_object = clCreateBuffer(OpenCLRuntime::_context, CL_MEM_READ_WRITE, buffer_size, data, &err);

        if(err == CL_SUCCESS)
        {
            ReturnType_t* pType = const_cast<ReturnType_t*>(&Type);
            cl_ulong* pWidth = const_cast<cl_ulong*>(&Width);
            cl_ulong* pHeight = const_cast<cl_ulong*>(&Height);
            size_t* pBufferSize = const_cast<size_t*>(&BufferSize);

            *pType = type;
            *pWidth = width;
            *pHeight = height;
            *pBufferSize = buffer_size;
        }

        return err;   
    }
    
    sickl_int OpenCLBuffer2D::SetData(void* in_buffer)
    {
        return clEnqueueWriteBuffer(OpenCLRuntime::_command_queue, _memory_object, true, 0, BufferSize, in_buffer, 0, nullptr, nullptr);
    }

    sickl_int OpenCLBuffer2D::GetData(void* out_buffer)
    {
        return clEnqueueReadBuffer(OpenCLRuntime::_command_queue, _memory_object, true, 0, BufferSize, out_buffer, 0, nullptr, nullptr);
    }
    
    //
    // OpenCLProgram
    //
    
    OpenCLProgram::OpenCLProgram()
        : _types(nullptr)
        , _type_count(0)
        , _param_index(0) 
        , _kernel(nullptr)
        , _dimension_count(0)
    { 
        _work_dimensions[0] = 0;
        _work_dimensions[1] = 0;
        _work_dimensions[2] = 0;
    }
    
    void OpenCLProgram::Delete()
    {
        if(_kernel != nullptr)
        {
            SICKL_ASSERT(clReleaseKernel(_kernel) == CL_SUCCESS);
            _kernel = nullptr;
        }
        delete[] _types;
        _types = nullptr;
        _type_count = 0;
        _dimension_count = 0;
        _work_dimensions[0] = 0;
        _work_dimensions[1] = 0;
        _work_dimensions[2] = 0;
    }
    
#define VALIDATE_ARG(ARG, TYPE) \
    template<> \
    sickl_int OpenCLProgram::ValidateArg<ARG>(const ARG&, const ReturnType_t type) \
    { \
        SICKL_ASSERT(type == TYPE); \
        ReturnErrorIfFalse(type == TYPE, SICKL_INVALID_KERNEL_ARG); \
        return SICKL_SUCCESS; \
    }
    
    // primitive types
    VALIDATE_ARG(bool, ReturnType::Bool)   
    
    VALIDATE_ARG(int32_t, ReturnType::Int)
    VALIDATE_ARG(int2, ReturnType::Int2)
    VALIDATE_ARG(int3, ReturnType::Int3)
    VALIDATE_ARG(int4, ReturnType::Int4)
    
    VALIDATE_ARG(uint32_t, ReturnType::UInt)
    VALIDATE_ARG(uint2, ReturnType::UInt2)
    VALIDATE_ARG(uint3, ReturnType::UInt3)
    VALIDATE_ARG(uint4, ReturnType::UInt4)
    
    VALIDATE_ARG(float, ReturnType::Float)
    VALIDATE_ARG(float2, ReturnType::Float2)
    VALIDATE_ARG(float3, ReturnType::Float3)
    VALIDATE_ARG(float4, ReturnType::Float4)
    
#undef VALIDATE_ARG    
    
#define VALIDATE_BUFFER_ARG(BUFF, BUFF_TYPE) \
    template<> \
    sickl_int OpenCLProgram::ValidateArg<BUFF>(const BUFF& buff, const ReturnType_t type) \
    { \
        SICKL_ASSERT(type & BUFF_TYPE); \
        SICKL_ASSERT((type ^ BUFF_TYPE) == buff.Type); \
        ReturnErrorIfFalse(type & BUFF_TYPE, SICKL_INVALID_KERNEL_ARG); \
        ReturnErrorIfFalse((type ^ BUFF_TYPE) == buff.Type, SICKL_INVALID_KERNEL_ARG); \
        return SICKL_SUCCESS; \
    }
    
    VALIDATE_BUFFER_ARG(OpenCLBuffer1D, ReturnType::Buffer1D)
    VALIDATE_BUFFER_ARG(OpenCLBuffer2D, ReturnType::Buffer2D)
    
#undef VALIDATE_BUFFER_ARG 
   
    template<>
    sickl_int OpenCLProgram::SetArg<OpenCLBuffer1D>(const OpenCLBuffer1D& buffer)
    {
        ReturnIfError(clSetKernelArg(_kernel, _arg_index++, sizeof(cl_uint), &buffer.Length));
        ReturnIfError(clSetKernelArg(_kernel, _arg_index++, sizeof(cl_mem), &buffer._memory_object));
        return SICKL_SUCCESS;
    }
    
    template<>
    sickl_int OpenCLProgram::SetArg<OpenCLBuffer2D>(const OpenCLBuffer2D& buffer)
    {
        ReturnIfError(clSetKernelArg(_kernel, _arg_index++, sizeof(cl_uint), &buffer.Width));
        ReturnIfError(clSetKernelArg(_kernel, _arg_index++, sizeof(cl_uint), &buffer.Height));
        ReturnIfError(clSetKernelArg(_kernel, _arg_index++, sizeof(cl_mem), &buffer._memory_object));
        
        return SICKL_SUCCESS;
    }
        

}
