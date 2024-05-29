/*

Sources: http://www.eriksmistad.no/getting-started-with-opencl-and-gpu-computing/

*/

// openCL headers
#include<iostream>
#include<stdio.h>
#include<stdlib.h>


#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif



#include <stdio.h>
#include <stdlib.h>


#define MAX_SOURCE_SIZE (0x100000)
#define SIZE 10 // number of processes



int main(int argc, char** argv) {
	srand(0);
	int array_size = 0;
	array_size = (rand() % 10000) + 1000; // randomizing array size
	int* Array = (int*)malloc(sizeof(int) * array_size); // original array
	int* Array2 = (int*)malloc(sizeof(int) * array_size); // original array

	int* displacement = (int*)malloc(sizeof(int) * SIZE);
	int* send_count = (int*)malloc(sizeof(int) * SIZE);
	int* result = (int*)malloc(sizeof(int) * SIZE);


	int part = array_size / SIZE;
	int temp = 0;
	for (int i = 0; i < SIZE - 1; i++)
	{
		displacement[i] = temp;
		send_count[i] = displacement[i] + part;
		temp += part + 1;
	}
	displacement[SIZE - 1] = temp;
	send_count[SIZE - 1] = array_size - 1;


	// initialising array
	for (int i = 0; i < array_size; i++)
	{
		Array[i] = rand()%10;
		Array2[i] = rand() % 10;
	}

	// Load kernel from file kernel.cl

	FILE* kernelFile;
	char* kernelSource;
	size_t kernelSize;

	kernelFile = fopen("kernel2.cl", "r");

	if (!kernelFile) {

		fprintf(stderr, "No file named kernel.cl was found\n");

		exit(-1);

	}
	kernelSource = (char*)malloc(MAX_SOURCE_SIZE);
	kernelSize = fread(kernelSource, 1, MAX_SOURCE_SIZE, kernelFile);
	fclose(kernelFile);

	// Getting platform and device information
	cl_platform_id platformId = NULL;
	cl_device_id deviceID = NULL;
	cl_uint retNumDevices;
	cl_uint retNumPlatforms;
	cl_int ret = clGetPlatformIDs(1, &platformId, &retNumPlatforms);
	char* value;
	size_t valueSize;

	ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_DEFAULT, 1, &deviceID, &retNumDevices);
	clGetDeviceInfo(deviceID, CL_DEVICE_NAME, 0, NULL, &valueSize);
	value = (char*)malloc(valueSize);
	clGetDeviceInfo(deviceID, CL_DEVICE_NAME, valueSize, value, NULL);
	printf("Device: %s\n", value);
	free(value);

	// Creating context.
	cl_context context = clCreateContext(NULL, 1, &deviceID, NULL, NULL, &ret);


	// Creating command queue
	cl_command_queue commandQueue = clCreateCommandQueue(context, deviceID, 0, &ret);

	// Memory buffers for each array
	cl_mem aMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, array_size * sizeof(int), NULL, &ret); // array
	cl_mem bMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, array_size * sizeof(int), NULL, &ret); // array second
	cl_mem cMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, SIZE * sizeof(int), NULL, &ret); // displacement
	cl_mem dMemObj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SIZE * sizeof(int), NULL, &ret); // send count
	cl_mem eMemObj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SIZE * sizeof(int), NULL, &ret); // result

	// Copy lists to memory buffers
	ret = clEnqueueWriteBuffer(commandQueue, aMemObj, CL_TRUE, 0, array_size * sizeof(int), Array, 0, NULL, NULL); // original
	ret = clEnqueueWriteBuffer(commandQueue, bMemObj, CL_TRUE, 0, array_size * sizeof(int), Array2, 0, NULL, NULL); // original second
	ret = clEnqueueWriteBuffer(commandQueue, cMemObj, CL_TRUE, 0, SIZE * sizeof(int), displacement, 0, NULL, NULL); // disp
	ret = clEnqueueWriteBuffer(commandQueue, dMemObj, CL_TRUE, 0, SIZE * sizeof(int), send_count, 0, NULL, NULL); // send

	// Create program from kernel source
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, (const size_t*)&kernelSize, &ret);

	// Build program
	ret = clBuildProgram(program, 1, &deviceID, NULL, NULL, NULL);

	// Create kernel
	cl_kernel kernel = clCreateKernel(program, "Mult", &ret);


	// Set arguments for kernel
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&aMemObj);
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&bMemObj);
	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&cMemObj);
	ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&dMemObj);
	ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&eMemObj);

	// Execute the kernel
	size_t globalItemSize = SIZE;
	size_t localItemSize = 1; // globalItemSize has to be a multiple of localItemSize. 1024/64 = 16 
	ret = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &globalItemSize, &localItemSize, 0, NULL, NULL);

	// Read from device back to host.
	ret = clEnqueueReadBuffer(commandQueue, dMemObj, CL_TRUE, 0, SIZE * sizeof(int), result, 0, NULL, NULL);

	// Write result
	int sum = 0;
	for (int i = 0; i < SIZE; i++)
	{
		sum += result[i];
	}

	printf("Sum: %d\n", sum);
/*	for (int i = 0; i < SIZE; i++)
	{
		printf("Sum: %d\n", result[i]);
	}
*/	// Test if correct answer
	int TSum = 0;
	for (int i = 0; i < array_size; i++)
	{
		TSum += (Array[i]*Array2[i]);
	}
	printf("TSum: %d\n", sum);

	// Clean up, release memory.
	ret = clFlush(commandQueue);
	ret = clFinish(commandQueue);
	ret = clReleaseCommandQueue(commandQueue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(aMemObj);
	ret = clReleaseMemObject(bMemObj);
	ret = clReleaseMemObject(cMemObj);
	ret = clReleaseMemObject(dMemObj);
	ret = clReleaseMemObject(eMemObj);
	ret = clReleaseContext(context);
	free(displacement);
	free(send_count);
	free(result);

	return 0;

}