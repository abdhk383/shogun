/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2012 Heiko Strathmann
 */

#include <shogun/statistics/KernelTwoSampleTestStatistic.h>
#include <shogun/features/Features.h>
#include <shogun/kernel/Kernel.h>

using namespace shogun;

CKernelTwoSampleTestStatistic::CKernelTwoSampleTestStatistic() :
		CTwoSampleTestStatistic()
{
	init();
}

CKernelTwoSampleTestStatistic::CKernelTwoSampleTestStatistic(CKernel* kernel,
		CFeatures* p_and_q, index_t q_start) :
		CTwoSampleTestStatistic(p_and_q, q_start)
{
	init();

	m_kernel=kernel;
	SG_REF(kernel);
}

CKernelTwoSampleTestStatistic::~CKernelTwoSampleTestStatistic()
{
	SG_UNREF(m_kernel);
}

void CKernelTwoSampleTestStatistic::init()
{
	/* TODO register params */
	m_kernel=NULL;
}
