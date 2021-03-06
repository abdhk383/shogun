/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2012 Heiko Strathmann
 */

#include <shogun/statistics/QuadraticTimeMMD.h>
#include <shogun/features/Features.h>
#include <shogun/mathematics/Statistics.h>

using namespace shogun;

CQuadraticTimeMMD::CQuadraticTimeMMD() : CKernelTwoSampleTestStatistic()
{
	init();
}

CQuadraticTimeMMD::CQuadraticTimeMMD(CKernel* kernel, CFeatures* p_and_q,
		index_t q_start) :
		CKernelTwoSampleTestStatistic(kernel, p_and_q, q_start)
{
	init();

	if (q_start!=p_and_q->get_num_vectors()/2)
	{
		SG_ERROR("CQuadraticTimeMMD: Only features with equal number of vectors "
				"are currently possible\n");
	}
}

CQuadraticTimeMMD::~CQuadraticTimeMMD()
{

}

void CQuadraticTimeMMD::init()
{
	/* TODO register parameters*/
	m_num_samples_spectrum=0;
	m_num_eigenvalues_spectrum=0;
}

float64_t CQuadraticTimeMMD::compute_statistic()
{
	/* split computations into three terms from JLMR paper (see documentation )*/
	index_t m=m_q_start;
	index_t n=m_p_and_q->get_num_vectors();

	/* init kernel with features */
	m_kernel->init(m_p_and_q, m_p_and_q);

	/* first term */
	float64_t first=0;
	for (index_t i=0; i<m; ++i)
	{
		for (index_t j=0; j<m; ++j)
		{
			/* ensure i!=j */
			if (i==j)
				continue;

			first+=m_kernel->kernel(i,j);
		}
	}
	first/=m*(m-1);

	/* second term */
	float64_t second=0;
	for (index_t i=m_q_start; i<n; ++i)
	{
		for (index_t j=m_q_start; j<n; ++j)
		{
			/* ensure i!=j */
			if (i==j)
				continue;

			second+=m_kernel->kernel(i,j);
		}
	}
	second/=n*(n-1);

	/* third term */
	float64_t third=0;
	for (index_t i=0; i<m; ++i)
	{
		for (index_t j=m_q_start; j<n; ++j)
			third+=m_kernel->kernel(i,j);
	}
	third*=-2.0/(m*n);

	return first+second-third;
}

float64_t CQuadraticTimeMMD::compute_p_value(float64_t statistic)
{
	float64_t result=0;

	switch (m_p_value_method)
	{
#ifdef HAVE_LAPACK
	case MMD2_SPECTRUM:
	{
		/* get samples from null-distribution and compute p-value of statistic */
		SGVector<float64_t> null_samples=sample_null_spectrum(
				m_num_samples_spectrum, m_num_eigenvalues_spectrum);
		CMath::qsort(null_samples);
		float64_t pos=CMath::find_position_to_insert(null_samples, statistic);
		result=1.0-pos/null_samples.vlen;
		break;
	}
#endif // HAVE_LAPACK
	case MMD2_GAMMA:
		result=compute_p_value_gamma(statistic);
		break;
	default:
		result=CKernelTwoSampleTestStatistic::compute_p_value(statistic);
		break;
	}

	return result;
}

#ifdef HAVE_LAPACK
SGVector<float64_t> CQuadraticTimeMMD::sample_null_spectrum(index_t num_samples,
		index_t num_eigenvalues)
{
	/* the whole procedure is already checked against MATLAB implementation */

	if (m_q_start!=m_p_and_q->get_num_vectors()/2)
	{
		/* TODO support different numbers of samples */
		SG_ERROR("%s::sample_null_spectrum(): Currently, only equal "
				"sample sizes are supported\n", get_name());
	}

	if (num_samples<=2)
	{
		SG_ERROR("%s::sample_null_spectrum(): Number of samples has to be at"
				" least 2, better in the hundrets", get_name());
	}

	if (num_eigenvalues>2*m_q_start-1)
	{
		SG_ERROR("%s::sample_null_spectrum(): Number of Eigenvalues too large\n",
				get_name());
	}

	if (num_eigenvalues<1)
	{
		SG_ERROR("%s::sample_null_spectrum(): Number of Eigenvalues too small\n",
				get_name());
	}

	/* imaginary matrix K=[K KL; KL' L] (MATLAB notation)
	 * K is matrix for XX, L is matrix for YY, KL is XY, LK is YX
	 * works since X and Y are concatenated here */
	m_kernel->init(m_p_and_q, m_p_and_q);
	SGMatrix<float64_t> K=m_kernel->get_kernel_matrix();

	/* center matrix K=H*K*H */
	K.center();

	/* compute eigenvalues and select num_eigenvalues largest ones */
	SGVector<float64_t> eigenvalues=SGMatrix<float64_t>::compute_eigenvectors(K);
	SGVector<float64_t> largest_ev(num_eigenvalues);

	/* scale by 1/2/m on the fly and take abs value*/
	for (index_t i=0; i<num_eigenvalues; ++i)
		largest_ev[i]=CMath::abs(1.0/2/m_q_start*eigenvalues[eigenvalues.vlen-1-i]);

	/* finally, sample from null distribution */
	SGVector<float64_t> null_samples(num_samples);
	for (index_t i=0; i<num_samples; ++i)
	{
		/* 2*sum(kEigs.*(randn(length(kEigs),1)).^2); */
		null_samples[i]=0;
		for (index_t j=0; j<largest_ev.vlen; ++j)
			null_samples[i]+=largest_ev[j]*CMath::pow(2.0, 2);

		null_samples[i]*=2;
	}

	return null_samples;
}
#endif // HAVE_LAPACK

float64_t CQuadraticTimeMMD::compute_p_value_gamma(float64_t statistic)
{
	/* the whole procedure is already checked against MATLAB implementation */

	if (m_q_start!=m_p_and_q->get_num_vectors()/2)
	{
		/* TODO support different numbers of samples */
		SG_ERROR("%s::compute_threshold_gamma(): Currently, only equal "
				"sample sizes are supported\n", get_name());
	}

	/* imaginary matrix K=[K KL; KL' L] (MATLAB notation)
	 * K is matrix for XX, L is matrix for YY, KL is XY, LK is YX
	 * works since X and Y are concatenated here */
	m_kernel->init(m_p_and_q, m_p_and_q);
	SGMatrix<float64_t> K=m_kernel->get_kernel_matrix();

	/* compute mean under H0 of MMD, which is
	 * meanMMD  = 2/m * ( 1  - 1/m*sum(diag(KL))  );
	 * in MATLAB */
	float64_t mean_mmd=0;
	for (index_t i=0; i<m_q_start; ++i)
	{
		/* virtual KL matrix is in upper right corner of SHOGUN K matrix
		 * so this sums the diagonal of the matrix between X and Y*/
		mean_mmd+=K(i, m_q_start+i);

		/* remove diagonal of all pairs of kernel matrices on the fly */
		K(i, i)=0;
		K(m_q_start+i, m_q_start+i)=0;
		K(i, m_q_start+i)=0;
		K(m_q_start+i, i)=0;
	}
	mean_mmd=2.0/m_q_start*(1.0-1.0/m_q_start*mean_mmd);

	/* compute variance under H0 of MMD, which is
	 * varMMD = 2/m/(m-1) * 1/m/(m-1) * sum(sum( (K + L - KL - KL').^2 ));
	 * in MATLAB, so sum up all elements */
	float64_t var_mmd=0;
	for (index_t i=0; i<m_q_start; ++i)
	{
		for (index_t j=0; j<m_q_start; ++j)
		{
			float64_t to_add=0;
			to_add+=K(i, j);
			to_add+=K(m_q_start+i, m_q_start+j);
			to_add-=K(i, m_q_start+j);
			to_add-=K(m_q_start+i, j);
			var_mmd+=CMath::pow(to_add, 2);
		}
	}
	var_mmd*=2.0/m_q_start/(m_q_start-1)*1.0/m_q_start/(m_q_start-1);

	/* parameters for gamma distribution */
	float64_t a=CMath::pow(mean_mmd, 2)/var_mmd;
	float64_t b=var_mmd*m_q_start / mean_mmd;

	/* return: cdf('gam',statistic,al,bet) (MATLAB)
	 * which will get the position in the null distribution */
	return CStatistics::gamma_cdf(statistic, a, b);
}

void CQuadraticTimeMMD::set_num_samples_sepctrum(index_t num_samples_spectrum)
{
	m_num_samples_spectrum=num_samples_spectrum;
}

void CQuadraticTimeMMD::set_num_eigenvalues_spectrum(
		index_t num_eigenvalues_spectrum)
{
	m_num_eigenvalues_spectrum=num_eigenvalues_spectrum;
}
