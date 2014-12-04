#include "_reg_ReadWriteImage.h"
#include "_reg_blockMatching.h"
#include "_reg_tools.h"

#include"Kernel.h"
#include"Kernels.h"
#include "cuda/CudaPlatform.h"
#include "cuda/CudaContext.h"

#define EPS 0.000001

void compare(nifti_image *referenceImage, nifti_image *warpedImage, int* mask, _reg_blockMatchingParam *refParams) {

	_reg_blockMatchingParam *cpu = new _reg_blockMatchingParam();
	initialise_block_matching_method(referenceImage, cpu, 50, 50, 1, mask, false);
	block_matching_method(referenceImage, warpedImage, cpu, mask);

	float* cpuTargetData = static_cast<float*>(cpu->targetPosition);
	float* cpuResultData = static_cast<float*>(cpu->resultPosition);

	float* cudaTargetData = static_cast<float*>(refParams->targetPosition);
	float* cudaResultData = static_cast<float*>(refParams->resultPosition);

	double maxTargetDiff = /*reg_test_compare_arrays<float>(refParams->targetPosition, static_cast<float*>(target->data), refParams->definedActiveBlock * 3)*/0.0;
	double maxResultDiff = /*reg_test_compare_arrays<float>(refParams->resultPosition, static_cast<float*>(result->data), refParams->definedActiveBlock * 3)*/0.0;

	double targetSum[3] = /*reg_test_compare_arrays<float>(refParams->targetPosition, static_cast<float*>(target->data), refParams->definedActiveBlock * 3)*/{ 0.0, 0.0, 0.0 };
	double resultSum[3] = /*reg_test_compare_arrays<float>(refParams->resultPosition, static_cast<float*>(result->data), refParams->definedActiveBlock * 3)*/{ 0.0, 0.0, 0.0 };

	//a better test will be to sort the 3d points and test the diff of each one!
	/*for (unsigned int i = 0; i < refParams->definedActiveBlock*3; i++) {

	 printf("i: %d target|%f-%f| result|%f-%f|\n", i, cpuTargetData[i], cudaTargetData[i], cpuResultData[i], cudaResultData[i]);
	 }*/

	for (unsigned long i = 0; i < refParams->definedActiveBlock; i++) {

		float cpuTargetPt[3] = { cpuTargetData[3 * i + 0], cpuTargetData[3 * i + 1], cpuTargetData[3 * i + 2] };
		float cpuResultPt[3] = { cpuResultData[3 * i + 0], cpuResultData[3 * i + 1], cpuResultData[3 * i + 2] };

		bool found = false;
		for (unsigned long j = 0; j < refParams->definedActiveBlock; j++) {
			float cudaTargetPt[3] = { cudaTargetData[3 * j + 0], cudaTargetData[3 * j + 1], cudaTargetData[3 * j + 2] };
			float cudaResultPt[3] = { cudaResultData[3 * j + 0], cudaResultData[3 * j + 1], cudaResultData[3 * j + 2] };

			targetSum[0] = cpuTargetPt[0] - cudaTargetPt[0];
			targetSum[1] = cpuTargetPt[1] - cudaTargetPt[1];
			targetSum[2] = cpuTargetPt[2] - cudaTargetPt[2];

			if (targetSum[0] == 0 && targetSum[1] == 0 && targetSum[2] == 0) {

				resultSum[0] = abs(cpuResultPt[0] - cudaResultPt[0]);
				resultSum[1] = abs(cpuResultPt[1] - cudaResultPt[1]);
				resultSum[2] = abs(cpuResultPt[2] - cudaResultPt[2]);
				found = true;
				if (resultSum[0] > 0.000001f || resultSum[1] > 0.000001f || resultSum[2] > 0.000001f)
					printf("i: %lu | j: %lu | (dif: %f-%f-%f) | (out: %f, %f, %f) | (ref: %f, %f, %f)\n", i, j, resultSum[0], resultSum[1], resultSum[2], cpuResultPt[0], cpuResultPt[1], cpuResultPt[2], cudaResultPt[0], cudaResultPt[1], cudaResultPt[2]);

			}
		}
		if (!found)
			printf("i: %lu has no match\n", i);
		/*double targetDiff = abs(refTargetPt[0] - outTargetPt[0]) + abs(refTargetPt[1] - outTargetPt[1]) + abs(refTargetPt[2] - outTargetPt[2]);
		 double resultDiff = abs(refResultPt[0] - outResultPt[0]) + abs(refResultPt[1] - outResultPt[1]) + abs(refResultPt[2] - outResultPt[2]);

		 maxTargetDiff = (targetDiff > maxTargetDiff) ? targetDiff : maxTargetDiff;
		 maxResultDiff = (resultDiff > maxResultDiff) ? resultDiff : maxResultDiff;*/
	}
}

void test(Context* con) {

	Platform *cudaPlatform = new CudaPlatform();

	Kernel* blockMatchingKernel = cudaPlatform->createKernel(BlockMatchingKernel::getName(), con);
	blockMatchingKernel->castTo<BlockMatchingKernel>()->calculate();

	delete blockMatchingKernel;
	delete cudaPlatform;
}
int main(int argc, char **argv) {

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <refImage> <warImage> <transType>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *inputRefImageName = argv[1];
	char *inputWarImageName = argv[2];
	int transType = atoi(argv[3]);

	// Read the input reference image
	nifti_image *referenceImage = reg_io_ReadImageFile(inputRefImageName);
	if (referenceImage == NULL) {
		reg_print_msg_error("The input reference image could not be read");
		return EXIT_FAILURE;
	}
	reg_tools_changeDatatype<float>(referenceImage);
	// Read the input floating image
	nifti_image *warpedImage = reg_io_ReadImageFile(inputWarImageName);
	if (warpedImage == NULL) {
		reg_print_msg_error("The input floating image could not be read");
		return EXIT_FAILURE;
	}
	reg_tools_changeDatatype<float>(warpedImage);

	// Create a mask
	int *mask = (int *) malloc(referenceImage->nvox * sizeof(int));
	for (size_t i = 0; i < referenceImage->nvox; ++i)
		mask[i] = i;

	Context* con = new CudaContext(referenceImage, NULL, mask, sizeof(float), 50, 50, 1);
	con->setCurrentWarped(warpedImage);
	test(con);

	_reg_blockMatchingParam *blockMatchingParams = con->getBlockMatchingParams();

//	compare(referenceImage, warpedImage, mask, blockMatchingParams);

	mat44 recoveredTransformation;
	reg_mat44_eye(&recoveredTransformation);
	recoveredTransformation.m[0][0] = 1.02f;
	recoveredTransformation.m[1][1] = 0.98f;
	recoveredTransformation.m[2][2] = 0.98f;
	recoveredTransformation.m[0][3] = 4.f;
	recoveredTransformation.m[1][3] = 4.f;
	recoveredTransformation.m[2][3] = 4.f;
	optimize(blockMatchingParams, &recoveredTransformation, transType);

	mat44 rigid2D;
	rigid2D.m[0][0] = 1.027961f;
	rigid2D.m[0][1] = -0.004180538f;
	rigid2D.m[0][2] = 0.f;
	rigid2D.m[0][3] = 3.601387f;
	rigid2D.m[1][0] = 0.01252018f;
	rigid2D.m[1][1] = 0.9764945f;
	rigid2D.m[1][2] = 0.f;
	rigid2D.m[1][3] = 3.17229f;
	rigid2D.m[2][0] = 0.f;
	rigid2D.m[2][1] = 0.f;
	rigid2D.m[2][2] = 1.f;
	rigid2D.m[2][3] = 0.f;
	rigid2D.m[3][0] = 0.f;
	rigid2D.m[3][1] = 0.f;
	rigid2D.m[3][2] = 0.f;
	rigid2D.m[3][3] = 1.f;

	mat44 rigid3D;
	rigid3D.m[0][0] = 1.028082f;
	rigid3D.m[0][1] = -0.004869822f;
	rigid3D.m[0][2] = 0.007795987f;
	rigid3D.m[0][3] = 4.177487f;
	rigid3D.m[1][0] = 0.01129405f;
	rigid3D.m[1][1] = 0.9697745f;
	rigid3D.m[1][2] = 0.005026158f;
	rigid3D.m[1][3] = 3.874551f;
	rigid3D.m[2][0] = 0.004100456f;
	rigid3D.m[2][1] = 0.01087017f;
	rigid3D.m[2][2] = 1.005741f;
	rigid3D.m[2][3] = 4.011357;
	rigid3D.m[3][0] = 0.f;
	rigid3D.m[3][1] = 0.f;
	rigid3D.m[3][2] = 0.f;
	rigid3D.m[3][3] = 1.f;

	mat44 affine2D;
	affine2D.m[0][0] = 0.9999999f;
	affine2D.m[0][1] = 0.0003671125f;
	affine2D.m[0][2] = 0.f;
	affine2D.m[0][3] = 3.652262f;
	affine2D.m[1][0] = -0.0003671125f;
	affine2D.m[1][1] = 0.9999999f;
	affine2D.m[1][2] = 0.f;
	affine2D.m[1][3] = 3.319299f;
	affine2D.m[2][0] = 0.f;
	affine2D.m[2][1] = 0.f;
	affine2D.m[2][2] = 1.f;
	affine2D.m[2][3] = 0.f;
	affine2D.m[3][0] = 0.f;
	affine2D.m[3][1] = 0.f;
	affine2D.m[3][2] = 0.f;
	affine2D.m[3][3] = 1.f;

	mat44 affine3D;
	affine3D.m[0][0] = 0.9999814f;
	affine3D.m[0][1] = -0.004359253f;
	affine3D.m[0][2] = 0.004272044f;
	affine3D.m[0][3] = 4.355269f;
	affine3D.m[1][0] = 0.004345424f;
	affine3D.m[1][1] = 0.9999853f;
	affine3D.m[1][2] = 0.003243448f;
	affine3D.m[1][3] = 4.134418f;
	affine3D.m[2][0] = -0.004286081f;
	affine3D.m[2][1] = -0.00322482f;
	affine3D.m[2][2] = 0.9999856f;
	affine3D.m[2][3] = 3.725645f;
	affine3D.m[3][0] = 0.f;
	affine3D.m[3][1] = 0.f;
	affine3D.m[3][2] = 0.f;
	affine3D.m[3][3] = 1.f;

	mat44 *testMatrix = NULL;
	if (referenceImage->nz > 1) {
		if (transType == 0)
			testMatrix = &affine3D;
		else
			testMatrix = &rigid3D;
	} else {
		if (transType == 0)
			testMatrix = &affine2D;
		else
			testMatrix = &rigid2D;

	}
#ifndef NDEBUG
	reg_mat44_disp(testMatrix, (char *) "expected");
	reg_mat44_disp(&recoveredTransformation, (char *) "recovered");
#endif

	mat44 differenceMatrix = *testMatrix - recoveredTransformation;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (fabsf(differenceMatrix.m[i][j]) > EPS) {
				fprintf(stderr, "reg_test_fullAffine error too large: %g (>%g) [%i,%i]\n", fabs(differenceMatrix.m[i][j]), EPS, i, j);
				return EXIT_FAILURE;
			}
		}
	}

	free(mask);
	nifti_image_free(referenceImage);
	delete con;

	return EXIT_SUCCESS;
}
