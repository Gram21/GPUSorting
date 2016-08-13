//#define MAX_LOCAL_SIZE 256 //set via compile options

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// needed helper methods
inline void swap(uint *a, uint *b) {
	uint tmp;
	tmp = *b;
	*b = *a;
	*a = tmp;
}

// dir == 1 means ascending
inline void sort(uint *a, uint *b, char dir) {
	if ((*a > *b) == dir) swap(a, b);
}

inline void swapLocal(__local uint *a, __local uint *b) {
	uint tmp;
	tmp = *b;
	*b = *a;
	*a = tmp;
}

// dir == 1 means ascending
inline void sortLocal(__local uint *a, __local uint *b, char dir) {
	if ((*a > *b) == dir) swapLocal(a, b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// basic kernel for mergesort start
__kernel void Sort_MergesortStart(const __global uint* inArray, __global uint* outArray)
{
	__local uint local_buffer[2][MAX_LOCAL_SIZE * 2];
	const uint lid = get_local_id(0);
	const uint index = get_group_id(0) * (MAX_LOCAL_SIZE * 2) + lid;
	char pong = 0;
	char ping = 1;

	// load into local buffer
	local_buffer[0][lid] = inArray[index];
	local_buffer[0][lid + MAX_LOCAL_SIZE] = inArray[index + MAX_LOCAL_SIZE];

	// merge sort
	for (unsigned int stride = 2; stride <= MAX_LOCAL_SIZE * 2; stride <<= 1) {
		ping = pong;
		pong = 1 - ping;
		uint leftBoundary = lid * stride;
		uint rightBoundary = leftBoundary + stride;

		uint middle = leftBoundary + (stride >> 1);
		uint left = leftBoundary, right = middle;
		barrier(CLK_LOCAL_MEM_FENCE);
		if (rightBoundary > MAX_LOCAL_SIZE * 2) continue;
#pragma unroll
		for (uint i = 0; i < stride; i++) {
			uint leftVal = local_buffer[ping][left];
			uint rightVal = local_buffer[ping][right];
			bool selectLeft = left < middle && (right >= rightBoundary || leftVal <= rightVal);

			local_buffer[pong][leftBoundary + i] = (selectLeft) ? leftVal : rightVal;

			left += selectLeft;
			right += 1 - selectLeft;
		}
	}

	//write back
	barrier(CLK_LOCAL_MEM_FENCE);
	outArray[index] = local_buffer[pong][lid];
	outArray[index + MAX_LOCAL_SIZE] = local_buffer[pong][lid + MAX_LOCAL_SIZE];
}

// For smaller strides so we can use local_buffer without getting into memory problems
__kernel void Sort_MergesortGlobalSmall(const __global uint* inArray, __global uint* outArray, const uint stride, const uint size)
{
	__local uint local_buffer[MAX_LOCAL_SIZE * 2];

	// within one stride merge the different parts
	const uint baseIndex = get_global_id(0) * stride;
	const uint baseLocalIndex = get_local_id(0) * 2;

	uint middle = baseIndex + (stride >> 1);
	uint left = baseIndex;
	uint right = middle;
	bool selectLeft = false;

	if ((baseIndex + stride) > size) return;

	local_buffer[baseLocalIndex + 1] = inArray[left];

#pragma unroll
	for (uint i = baseIndex; i < (baseIndex + stride); i++) {
		// check which value should be written out
		local_buffer[baseLocalIndex + (int)selectLeft] = (selectLeft) ? inArray[left] : inArray[right];
		selectLeft = left < middle && (right == (baseIndex + stride) || local_buffer[baseLocalIndex + 1] <= local_buffer[baseLocalIndex]);

		// write out
		outArray[i] = (selectLeft) ? local_buffer[baseLocalIndex + 1] : local_buffer[baseLocalIndex]; //PROBLEMATIC PART! WE RUN OUT OF MEMORY

		//increase counter accordingly
		left += selectLeft;
		right += 1 - selectLeft;
	}
}

__kernel void Sort_MergesortGlobalBig(const __global uint* inArray, __global uint* outArray, const uint stride, const uint size)
{
	//Problems: Breaks at large arrays. this version was stripped down (so little less performance but supports little bigger arrays)

	// within one stride merge the different parts
	const uint baseIndex = get_global_id(0) * stride;
	const char dir = 1;

	uint middle = baseIndex + (stride >> 1);
	uint left = baseIndex;
	uint right = middle;
	bool selectLeft;

	if ((baseIndex + stride) > size) return;

#pragma unroll
	for (uint i = baseIndex; i < (baseIndex + stride); i++) {
		// check which value should be written out
		selectLeft = (left < middle && (right == (baseIndex + stride) || inArray[left] <= inArray[right])) == dir;

		// write out
		outArray[i] = (selectLeft) ? inArray[left] : inArray[right];

		//increase counter accordingly
		left += selectLeft;
		right += 1 - selectLeft;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OLD and BASIC global Kernel
__kernel void Sort_SimpleSortingNetwork(const __global uint* inArray, __global uint* outArray, const uint offset, const uint size)
{
	// TO DO: pimp! Problem: Kernel gets called pretty often -> overhead!
	const uint index = (get_global_id(0) << 1) + offset;
	if (index + 1 >= size) return;

	uint left = inArray[index];
	uint right = inArray[index + 1];
	sort(&left, &right, 1);
	outArray[index] = left;
	outArray[index + 1] = right;

	// not needed if we use input=output array
	//bool additionalWork = (get_global_id(0) == 0) && offset;
	//if (additionalWork) { //alternative: if (offset) {..} only
	//	outArray[0] = inArray[0];
	//	outArray[size - 1] = inArray[size - 1];
	//}
}

/*
 * data		- the data input and output array
 * size		- size of the array
 * offset	- whether or not there will be an offset
 */
__kernel void Sort_SimpleSortingNetworkLocal(__global uint* data, const uint size, const uint offset)
{
	__local uint local_buffer[MAX_LOCAL_SIZE * 2];
	const uint limit = MAX_LOCAL_SIZE * 2;
	const uint lid = get_local_id(0);
	const uint locIdx = lid << 1;
	const uint index = get_group_id(0) * (MAX_LOCAL_SIZE * 2) + (offset * MAX_LOCAL_SIZE) + lid;
	if (index + MAX_LOCAL_SIZE >= size) return;

	//load into local buffer
	local_buffer[lid] = data[index];
	local_buffer[lid + MAX_LOCAL_SIZE] = data[index + MAX_LOCAL_SIZE];

	//sort
#pragma unroll
	for (int i = 0; i < limit; i++) {
		barrier(CLK_LOCAL_MEM_FENCE);
		uint index = locIdx + (i & 1);
		if (index + 1 >= limit) continue;
		sortLocal(&local_buffer[index], &local_buffer[index + 1], 1);
	}

	// sync and write back
	barrier(CLK_LOCAL_MEM_FENCE);
	data[index] = local_buffer[lid];
	data[index + MAX_LOCAL_SIZE] = local_buffer[lid + MAX_LOCAL_SIZE];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void Sort_BitonicMergesortStart(const __global uint* inArray, __global uint* outArray)
{
	__local uint local_buffer[MAX_LOCAL_SIZE * 2];
	const uint gid = get_global_id(0);
	const uint lid = get_local_id(0);

	uint index = get_group_id(0) * (MAX_LOCAL_SIZE * 2) + lid;
	//load into local mem
	local_buffer[lid] = inArray[index];
	local_buffer[lid + MAX_LOCAL_SIZE] = inArray[index + MAX_LOCAL_SIZE];

	uint clampedGID = gid & (MAX_LOCAL_SIZE - 1);

	// bitonic merge
	for (uint blocksize = 2; blocksize < MAX_LOCAL_SIZE * 2; blocksize <<= 1) {
		char dir = (clampedGID & (blocksize / 2)) == 0; // sort every other block in the other direction (faster % calc)
#pragma unroll
		for (uint stride = blocksize >> 1; stride > 0; stride >>= 1){
			barrier(CLK_LOCAL_MEM_FENCE);
			uint idx = 2 * lid - (lid & (stride - 1)); //take every other input BUT starting neighbouring within one block
			sortLocal(&local_buffer[idx], &local_buffer[idx + stride], dir);
		}
	}

	// bitonic merge for biggest group is special (unrolling this so we dont need ifs in the part above)
	char dir = (clampedGID & 0); //even or odd? sort accordingly
#pragma unroll
	for (uint stride = MAX_LOCAL_SIZE; stride > 0; stride >>= 1){
		barrier(CLK_LOCAL_MEM_FENCE);
		uint idx = 2 * lid - (lid & (stride - 1));
		sortLocal(&local_buffer[idx], &local_buffer[idx + stride], dir);
	}

	// sync and write back
	barrier(CLK_LOCAL_MEM_FENCE);
	outArray[index] = local_buffer[lid];
	outArray[index + MAX_LOCAL_SIZE] = local_buffer[lid + MAX_LOCAL_SIZE];
}

__kernel void Sort_BitonicMergesortLocal(__global uint* data, const uint size, const uint blocksize, uint stride)
{
	// This Kernel is basically the same as Sort_BitonicMergesortStart except of the "unrolled" part and the provided parameters
	__local uint local_buffer[2 * MAX_LOCAL_SIZE];
	uint gid = get_global_id(0);
	uint groupId = get_group_id(0);
	uint lid = get_local_id(0);
	uint clampedGID = gid & (size / 2 - 1);

	uint index = groupId * (MAX_LOCAL_SIZE * 2) + lid;
	//load into local mem
	local_buffer[lid] = data[index];
	local_buffer[lid + MAX_LOCAL_SIZE] = data[index + MAX_LOCAL_SIZE];

	// bitonic merge
	char dir = (clampedGID & (blocksize / 2)) == 0; //same as above, % calc
#pragma unroll
	for (; stride > 0; stride >>= 1) {
		barrier(CLK_LOCAL_MEM_FENCE);
		uint idx = 2 * lid - (lid & (stride - 1));
		sortLocal(&local_buffer[idx], &local_buffer[idx + stride], dir);
	}

	// sync and write back
	barrier(CLK_LOCAL_MEM_FENCE);
	data[index] = local_buffer[lid];
	data[index + MAX_LOCAL_SIZE] = local_buffer[lid + MAX_LOCAL_SIZE];
}

__kernel void Sort_BitonicMergesortGlobal(__global uint* data, const uint size, const uint blocksize, const uint stride)
{
	// TO DO: Kernel implementation
	uint gid = get_global_id(0);
	uint clampedGID = gid & (size / 2 - 1);

	//calculate index and dir like above
	uint index = 2 * clampedGID - (clampedGID & (stride - 1));
	char dir = (clampedGID & (blocksize / 2)) == 0; //same as above, % calc

	//bitonic merge
	uint left = data[index];
	uint right = data[index + stride];

	sort(&left, &right, dir);

	// writeback
	data[index] = left;
	data[index + stride] = right;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
