#pragma once

#include "SiCKL.h"

namespace SiCKL
{
    
    template<typename T>
    struct vector2
    {
        T x;
        T y;
    };
    
    template<typename T>
    struct vector3 : public vector2<T>
    {
        T z;
    };
    
    template<typename T>
    struct vector4 : public vector3<T>
    {
        T w;
    };

    typedef vector2<int32_t> int2;
    typedef vector3<int32_t> int3;
    typedef vector4<int32_t> int4;
    
    typedef vector2<uint32_t> uint2;
    typedef vector3<uint32_t> uint3;
    typedef vector4<uint32_t> uint4;
    
    typedef vector2<float> float2;
    typedef vector3<float> float3;
    typedef vector4<float> float4;
    
    static_assert(sizeof(float2) == 2 * sizeof(float), "");
    static_assert(sizeof(float3) == 3 * sizeof(float), "");
    static_assert(sizeof(float4) == 4 * sizeof(float), "");
    
    class OpenCLRuntime
    {
    public:
        // setup opencl runtime
        static sickl_int Initialize();
        // tear it down
        static sickl_int Finalize();
    private:
        friend struct OpenCLBuffer1D;
        friend struct OpenCLBuffer2D;
        friend struct OpenCLProgram;
        
        static cl_context _context;
        static cl_device_id _device;
        static cl_command_queue _command_queue;
    };

    struct OpenCLBuffer1D : public RefCounted<OpenCLBuffer1D>
    {
        REF_COUNTED(OpenCLBuffer1D)

        OpenCLBuffer1D();
        sickl_int Initialize(size_t length, ReturnType_t, void* data);
        sickl_int SetData(void* in_buffer);
        sickl_int GetData(void* out_buffer);

        const ReturnType_t Type;
        const cl_ulong Length;
        const size_t BufferSize;
    private:
        cl_mem _memory_object;
        
        friend struct OpenCLProgram;
    };

    struct OpenCLBuffer2D : public RefCounted<OpenCLBuffer2D>
    {
        REF_COUNTED(OpenCLBuffer2D)
        
        OpenCLBuffer2D();
        sickl_int Initialize(size_t width, size_t height, ReturnType_t type, void* data);
        sickl_int SetData(void* in_buffer);
        sickl_int GetData(void* out_buffer);
        
        const ReturnType_t Type;
        const cl_ulong Width;
        const cl_ulong Height;
        const size_t BufferSize;
       private:
        cl_mem _memory_object;
        
        friend struct OpenCLProgram;
    };

    struct OpenCLProgram : public RefCounted<OpenCLProgram>
    {
        REF_COUNTED(OpenCLProgram)
    public:
        OpenCLProgram();
         
        sickl_int SetWorkDimensions(size_t length);
        sickl_int SetWorkDimensions(size_t length, size_t width);
        sickl_int SetWorkDimensions(size_t length, size_t width, size_t height);
        
        template<typename...Args>
        sickl_int operator()(const Args&... args)
        {
            _param_index = 0;
            _arg_index = 0;
            return Run(args...);
        }

    private:
        
        template<typename T>
        sickl_int ValidateArg(const T& arg, const ReturnType_t type);
        
        template<typename T>
        sickl_int SetArg(const T& arg)
        {
            return clSetKernelArg(_kernel, _arg_index++, sizeof(T), &arg);
        }
        
        sickl_int Run()
        {
            //return clEnqueueNDRangeKernel()
            return SICKL_SUCCESS;
        }
        
        template<typename Arg, typename...Args>
        sickl_int Run(const Arg& arg, const Args&... args)
        {;
            ReturnType_t type = _types[_param_index];
            // make sure this arg matches the required type
            ReturnIfError(ValidateArg(arg, type));
            
            // pass the arg to our kernel
            ReturnIfError(SetArg(arg));
                
            ++_param_index;
            ReturnIfError(Run(args...));
            
            return SICKL_SUCCESS;
        }
        
        // used to ensure our passed in args match the required types
        ReturnType_t* _types;
        size_t _type_count;
        // counter used in Run(...)
        size_t _param_index;
        // counter used in SetArg
        cl_uint _arg_index;
        // our built kernel
        cl_kernel _kernel;
        
        // work diemnsions
        size_t _work_dimensions[3];
        size_t _dimension_count;
    };

    class OpenCLCompiler
    {
    public:
        static sickl_int Build(SiCKL::Source& source, OpenCLProgram& program);
    };
}
