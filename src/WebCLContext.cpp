/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLContext.h"
#include "WebCLMemory.h"
#include "WebCLProgram.h"
#include "WebCLDevice.h"
#include "WebCLCommandQueue.h"
#include "WebCLEvent.h"
#include "WebCLSampler.h"
#include "WebCLPlatform.h"

#include <iostream>
using namespace std;

using namespace std;
using namespace v8;
using namespace webcl;
using namespace cl;

Persistent<FunctionTemplate> WebCLContext::constructor_template;

/* static  */
void WebCLContext::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLContext::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLContext"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createProgramWithSource", createProgramWithSource);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createCommandQueue", createCommandQueue);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createBuffer", createBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createImage2D", createImage2D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createImage3D", createImage3D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createSampler", createSampler);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createUserEvent", createUserEvent);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getSupportedImageFormats", getSupportedImageFormats);

  target->Set(String::NewSymbol("WebCLContext"), constructor_template->GetFunction());
}

WebCLContext::WebCLContext(Handle<Object> wrapper) : context(0)
{
}

WebCLContext::~WebCLContext()
{
  if (context) delete context;
}

/* static */
JS_METHOD(WebCLContext::getInfo)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_context_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_CONTEXT_REFERENCE_COUNT:
  case CL_CONTEXT_NUM_DEVICES: {
    cl_uint param_value=0;
    cl_int ret=context->getContext()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_CONTEXT_DEVICES: {
    VECTOR_CLASS<cl_device_id>  ctx;
    context->getContext()->getInfo(param_name,&ctx);

    int n=ctx.size();
    Local<Array> arr = Array::New(n);
    for(int i=0;i<n;i++) {
      cl::Device *device=new cl::Device(ctx[i]);
      arr->Set(i,WebCLDevice::New(device)->handle_);
    }
    return scope.Close(arr);
  }
  case CL_CONTEXT_PROPERTIES: {
    VECTOR_CLASS<cl_context_properties>  ctx;
    context->getContext()->getInfo(param_name,&ctx);

    int n=ctx.size();
    Local<Array> arr = Array::New(n);
    for(int i=0;i<n;i++) {
      arr->Set(i,JS_INT(ctx[i]));
    }
    return scope.Close(arr);
  }
  default:
    return ThrowException(Exception::Error(String::New("UNKNOWN param_name")));
  }
}

// TODO change to createProgram(String) and createProgram(Buffer)
JS_METHOD(WebCLContext::createProgramWithSource)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  Local<String> str = args[0]->ToString();
  String::AsciiValue astr(str);
  cl::Program::Sources sources;
  std::pair<const char*, ::size_t> source((*astr),astr.length());
  sources.push_back(source);

  cl_int ret = CL_SUCCESS;
  cl::Program *pw=new cl::Program(*context->getContext(),sources,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLProgram::New(pw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createCommandQueue)
{
  HandleScope scope;
  WebCLContext *context = ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl::Device *device = ObjectWrap::Unwrap<WebCLDevice>(args[0]->ToObject())->getDevice();
  cl_command_queue_properties properties = args[1]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::CommandQueue *cw = new cl::CommandQueue(*context->getContext(), *device, properties,&ret);

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_QUEUE_PROPERTIES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLCommandQueue::New(cw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createBuffer)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->Uint32Value();
  ::size_t size = args[1]->Uint32Value();
  void *host_ptr=0; //node::ObjectWrap::Unwrap<CLContext>(args[0]->ToObject()); // TODO what is this host_ptr? a TypedArray?

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Buffer(*context->getContext(),flags,size,host_ptr,&ret);

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_BUFFER_SIZE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_HOST_PTR);
    WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createImage2D)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();

  cl::ImageFormat image_format;
  Local<Object> obj = args[1]->ToObject();
  image_format.image_channel_order = obj->Get(JS_STR("order"))->Uint32Value();
  image_format.image_channel_data_type = obj->Get(String::New("data_type"))->Uint32Value();

  std::size_t width = args[2]->NumberValue();
  std::size_t height = args[3]->NumberValue();
  std::size_t row_pitch = args[4]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Image2D(*context->getContext(),flags,image_format,width,height,row_pitch,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    WEBCL_COND_RETURN_THROW(CL_INVALID_IMAGE_SIZE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_HOST_PTR);
    WEBCL_COND_RETURN_THROW(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_OPERATION);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createImage3D)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();

  cl::ImageFormat image_format;
  Local<Object> obj = args[1]->ToObject();
  image_format.image_channel_order = obj->Get(JS_STR("order"))->Uint32Value();
  image_format.image_channel_data_type = obj->Get(JS_STR("data_type"))->Uint32Value();

  std::size_t width = args[2]->NumberValue();
  std::size_t height = args[3]->NumberValue();
  std::size_t depth = args[4]->NumberValue();
  std::size_t row_pitch = args[5]->NumberValue();
  std::size_t slice_pitch = args[6]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Image3D(*context->getContext(),flags,image_format,width,height,depth,row_pitch,slice_pitch,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    WEBCL_COND_RETURN_THROW(CL_INVALID_IMAGE_SIZE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_HOST_PTR);
    WEBCL_COND_RETURN_THROW(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_OPERATION);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createSampler)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_bool norm_coords = args[0]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_addressing_mode addr_mode = args[1]->NumberValue();
  cl_filter_mode filter_mode = args[2]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Sampler *sw=new cl::Sampler(*context->getContext(),norm_coords,addr_mode,filter_mode,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_OPERATION);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLSampler::New(sw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::getSupportedImageFormats)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();
  cl_mem_object_type image_type = args[1]->NumberValue();
  VECTOR_CLASS<cl::ImageFormat> image_formats;

  cl_int ret = context->getContext()->getSupportedImageFormats(flags,image_type,&image_formats);

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  Local<Array> imageFormats = Array::New();
  for (int i=0; i<image_formats.size(); i++) {
    Local<Object> format = Object::New();
    format->Set(JS_STR("order"), JS_INT(image_formats[i].image_channel_order));
    format->Set(JS_STR("data_type"), JS_INT(image_formats[i].image_channel_data_type));
    imageFormats->Set(i, format);
  }

  return scope.Close(imageFormats);
}

/* static */
JS_METHOD(WebCLContext::createUserEvent)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());

  cl::UserEvent *ew=new cl::UserEvent();
  return scope.Close(WebCLEvent::New(ew)->handle_);
}

/* static  */
JS_METHOD(WebCLContext::New)
{
  HandleScope scope;
  if (!args[1]->IsArray()) {
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));
  }

  cl_device_type device_type = args[0]->Uint32Value();

  Local<Array> propertiesArray = Array::Cast(*args[1]);
  int num=propertiesArray->Length();
  cl_context_properties *properties=new cl_context_properties[num+1];
  for (int i=0; i<num; i+=2) {

    properties[i]=propertiesArray->Get(i)->Uint32Value();

    Local<Object> obj = propertiesArray->Get(i+1)->ToObject();
    WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(obj);
    properties[i+1]=(cl_context_properties) platform->getPlatform()->operator ()();
  }
  properties[num]=0;

  cl_int ret=CL_SUCCESS;
  cl::Context *cw = new cl::Context(device_type,properties,NULL,NULL,&ret);

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROPERTY);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE_TYPE);
    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_AVAILABLE);
    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_FOUND);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  WebCLContext *cl = new WebCLContext(args.This());
  cl->context=cw;
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLContext *WebCLContext::New(cl::Context* cw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLContext *context = ObjectWrap::Unwrap<WebCLContext>(obj);
  context->context = cw;

  return context;
}