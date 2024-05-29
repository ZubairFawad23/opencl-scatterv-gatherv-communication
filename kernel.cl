__kernel void addSum(__global const int *Array, 
	__global const int *displacement,
	__global const int *send_count,
	__global int * result) {
		
		int gid = get_global_id(0);
		int d = displacement[gid];
		int s = send_count[gid];
		int localS = 0;

		for(int i = d; i <= s; i++)
		{
			localS += Array[i];
		}
		result[gid] = localS;
	}
