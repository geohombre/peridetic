//
//
// MIT License
//
// Copyright (c) 2020 Stellacore Corporation.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//


#include "peridetic.h"

#include "periLocal.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>


namespace peri::sim
{
	//! Geometric sampling relationships for uniformly space sampling
	struct SampleSpec
	{
		std::size_t const theNum{}; //!< \note MUST satisfy (1<theNum)
		std::pair<double, double> const theRange{};

		//! Number of samples generated by this spec
		inline
		std::size_t
		size
			() const
		{
			return theNum;
		}

		//! Increment producing number of samples spanning *CLOSED* range
		inline
		double
		delta
			() const
		{
			double const rangeMag{ theRange.second - theRange.first };
			double const delta{ (1./static_cast<double>(theNum-1)) * rangeMag };
			return delta;
		}

		//! The first value that is *IN*cluded - i.e. start of range
		inline
		double
		begin
			() const
		{
			return theRange.first;
		}

		//! The last value that is *IN*cluded - i.e. end of range
		inline
		double
		last
			() const
		{
			return theRange.second;
		}

		//! Value associated with sampling index (\note NO checking on ndx)
		inline
		double
		valueAtIndex
			( std::size_t const & ndx
			) const
		{
			return begin() + static_cast<double>(ndx) * delta();
		}

	}; // SampleSpec

	//! Collection of samples generated per specification
	inline
	std::vector<double>
	samplesFor
		( SampleSpec const & spec
		)
	{
		std::vector<double> samps;
		samps.reserve(spec.size());
		for (std::size_t ndx{0u} ; ndx < spec.size() ; ++ndx)
		{
			double const value{ spec.valueAtIndex(ndx) };
			samps.emplace_back(value);
		}
		return samps;
	}


	//! Collection of samples generated per specification
	struct Samples
	{
		std::vector<double> const theSamps{};

		inline
		static
		std::vector<double>
		samplesFor
			( SampleSpec const & spec
			)
		{
			std::vector<double> samps;
			samps.reserve(spec.size());
			for (std::size_t ndx{0u} ; spec.size() ; ++ndx)
			{
				double const value{ spec.valueAtIndex(ndx) };
				samps.emplace_back(value);
			}
			return samps;
		}

		//! Populate internal collection
		inline
		explicit
		Samples
			( SampleSpec const & spec
			)
			: theSamps{ samplesFor(spec) }
		{ }

	}; // Samples

	//! Samples covering meridian plane (spherically distributed)
	std::vector<XYZ>
	meridianPlaneSamples
		( SampleSpec const & radSpec
		, SampleSpec const & parSpec
		, double const & lonVal = .25*pi() 
			//!< generate plane at this longitude
		)
	{
		std::vector<XYZ> xyzs{};
		std::vector<double> const parVals{ samplesFor(parSpec) };
		std::vector<double> const radVals{ samplesFor(radSpec) };
		xyzs.reserve(parVals.size() * radVals.size());
		for (double const & parVal : parVals)
		{
			XYZ const xyzDir
				{ std::cos(parVal) * std::cos(lonVal)
				, std::cos(parVal) * std::sin(lonVal)
				, std::sin(parVal)
				};
			for (double const & radVal : radVals)
			{
				XYZ const xyz{ radVal * xyzDir };
				xyzs.emplace_back(xyz);
			}
		}
		return xyzs;
	}

} // [peri::sim]


namespace
{
	int
	test1
		( std::ostream & ostrm
		, peri::sim::SampleSpec const & radSpec
		, peri::sim::SampleSpec const & parSpec
		, peri::EarthModel const & earth
		, bool const & showSamples = false
		)
	{
		int errCount{ 0u };

		peri::Shape const & shape = earth.theEllip.theShapeOrig;

		using namespace  peri::sim;
		std::vector<peri::XYZ> const xyzs
			{ meridianPlaneSamples(radSpec, parSpec) };

		std::vector<double> extras{};
		extras.reserve(xyzs.size());
		for (peri::XYZ const & xyz : xyzs)
		{
			peri::XYZ const & xVec = xyz;
			peri::LPA const xLpa{ peri::lpaForXyz(xVec, earth) };
			double const & eta = xLpa[2];

			peri::XYZ const xDir{ peri::unit(xVec) };

			peri::LPA const pLpa{ xLpa[0], xLpa[1], 0. };
			peri::XYZ const pVec{ peri::xyzForLpa(pLpa, earth) };

			double const rMag{ peri::ellip::radiusToward(xDir, shape) };
			using peri::operator*;
			peri::XYZ const rVec{ rMag * xDir };

			using peri::operator-;
			peri::XYZ const xrVec{ xVec - rVec };
			peri::XYZ const xpVec{ xVec - pVec };

			double const xrMag{ peri::magnitude(xrVec) };
			double const xpMag{ peri::magnitude(xpVec) };

			double const extra{ xrMag - xpMag };
			extras.emplace_back(extra);

			// gradients
			peri::XYZ const gpVec{ shape.gradientAt(pVec) };
			double const gpMag{ peri::magnitude(gpVec) };
			peri::XYZ const grVec{ shape.gradientAt(rVec) };
			double const grMag{ peri::magnitude(grVec) };
			double const gRatio{ grMag / gpMag };
			double const rEps{ gRatio - 1. };
		//	peri::XYZ const gDifVec{ grVec - gpVec };
		//	double const gDifMag{ peri::magnitude(gDifVec) };

			// compute deltaEta, deltaSigma
			double const delEta{ xrMag - xpMag };
			double const dEtaPerR{ delEta / rMag };

			if (showSamples)
			{
				ostrm
				//	<< " xVec: " << peri::xyz::infoString(xVec)
				//	<< " xLpa: " << peri::lpa::infoString(xLpa)
			//		<< " Lon: " << peri::string::fixedAngular(xLpa[0])
					<< " Par: " << peri::string::fixedAngular(xLpa[1])
					<< " Alt: " << peri::string::allDigits(eta)
				//	<< " " << peri::string::fixedLinear(xrMag, "xrMag")
				//	<< " " << peri::string::fixedLinear(xpMag, "xpMag")
					<< " " << peri::string::fixedLinear(extra, "extra")
					<< " " << peri::string::allDigits(rEps, "rEps")
					<< " " << peri::string::allDigits(delEta, "delEta")
					<< " " << peri::string::allDigits(dEtaPerR, "dEtaPerR")
					<< std::endl;
			}
		}

		if (! extras.empty())
		{
			std::sort(extras.begin(), extras.end());
			using peri::string::fixedLinear;
			ostrm << "# minExcess: " << fixedLinear(extras.front()) << '\n';
			ostrm << "# maxExcess: " << fixedLinear(extras.back()) << '\n';
		}

++errCount;
		return errCount;
	}

	//! Coefficients for quadratic function in 'zeta' (corrections to alt)
	std::array<double, 3u>
	zetaCoefficients
		( peri::XYZ const & xVec
		, double const & eta0
		, double const & grMag
		, peri::Shape const & shape
		)
	{
		std::array<double, 3u> coABCs{ 0., 0., 0. };
		std::array<double, 3u> const & muSqs = shape.theMuSqs;
		double & coA = coABCs[0];
		double & coB = coABCs[1];
		double & coC = coABCs[2];
		for (std::size_t kk{0u} ; kk < 3u ; ++kk)
		{
			// compute common factor (and contributing element s1k)
			double const fgkInv{ .5 * grMag * muSqs[kk] };
			double const s1k{ 1. / (fgkInv + eta0) };
			double const n1k{ s1k * fgkInv * xVec[kk] };
			double const nPerMuSq{ peri::sq(n1k) / muSqs[kk] };
			// update coefficients
			coA += nPerMuSq * s1k * s1k;
			coB += nPerMuSq * s1k;
			coC += nPerMuSq;
		}
		// adjust coefficients with mulpliers and offsets
		coA *= 3.;
		coC -= 1.;
		return coABCs;
	}

	//! Compute point on ellipsoid, p, using perturbation expansion
	peri::XYZ
	pVecViaExcess
		( peri::XYZ const & xVec
		, peri::EarthModel const & earth
		)
	{
		using namespace peri;
		Shape const & shape = earth.theEllip.theShapeOrig;

		// radial point on ellipsoid
		double const rho{ peri::ellip::radiusToward(xVec, shape) };
		XYZ const rVec{ rho * unit(xVec) };

		// gradient at radial point
		XYZ const grVec{ shape.gradientAt(rVec) };
		double const grMag{ magnitude(grVec) };

		// radial pseudo-altitude
		double const eta0{ magnitude(xVec - rVec) };

		// TODO(name?) fraction components
		std::array<double, 3u> const & muSqs = shape.theMuSqs;

		std::array<double, 3u> const coABCs
			{ zetaCoefficients(xVec, eta0, grMag, shape) };
		double const & coA = coABCs[0];
		double const & coB = coABCs[1];
		double const & coC = coABCs[2];

		double const xArg{ (coA * coC) / sq(coB) };
	//	double const radical{ std::sqrt(1. - xArg) };
	//	double const zetaExact{ (coB / coA) * (1. - radical) };

		// approximations
		double const fracCoB{ coC / coB };
		// first order approximation
	//	double const zetaApx1
	//		{ fracCoB * (1./2.) };
		// second order approximation
		double const zetaApx2
			{ fracCoB * (1./2. + (1./8.)*xArg) };
	//	double const zetaApx3
	//		{ (.5 * fracCoB) * (1. + ((1./4.) + (1./8.)*xArg)*xArg) };

		double const & zeta = zetaApx2;
		double const correction{ 2. * (zeta + eta0) / grMag };

		XYZ const pVec
			{ xVec[0] / (1. + (correction/muSqs[0]))
			, xVec[1] / (1. + (correction/muSqs[1]))
			, xVec[2] / (1. + (correction/muSqs[2]))
			};

		return pVec;
	}


	//! Evaluate math equations for ellipsoidal excess at this point
	int
	checkXYZ
		( peri::XYZ const & xVecExp
		, peri::EarthModel const & earth
		, std::ofstream & ostrm
		)
	{
		using namespace peri;

		// compute pLocation based on perturbation expansion
		XYZ const pVecGot{ pVecViaExcess(xVecExp, earth) };

		// Quantities for checking values
		LPA const xLpaExp{ lpaForXyz(xVecExp, earth) };
		XYZ const pLpaExp{ xLpaExp[0], xLpaExp[1], 0. };
		XYZ const pVecExp{ xyzForLpa(pLpaExp, earth) };

		// Error amount
		XYZ const pVecDif{ pVecGot - pVecExp };
		double const pMagDif{ magnitude(pVecDif) };

		ostrm
			<< lpa::infoString(xLpaExp, "xLpaExp")
			<< " "
			<< xyz::infoString(pVecDif, "pVecDif")
			<< " "
			<< string::allDigits(pMagDif, "pMagDif")
			<< std::endl;
		return 0;
	}

	//! Check equations on sampling of points
	int
	test2
		( peri::sim::SampleSpec const & radSpec
		, peri::sim::SampleSpec const & parSpec
		, peri::EarthModel const & earth
		)
	{
		int errCount{ 0u };

		using namespace  peri::sim;
		std::vector<peri::XYZ> const xyzs
			{ meridianPlaneSamples(radSpec, parSpec) };
		std::ofstream ofsDifPVec("pvecDiff.dat");
		for (peri::XYZ const & xyz : xyzs)
		{
			errCount += checkXYZ(xyz, earth, ofsDifPVec);
		}
		return errCount;

	}

} // [annon]


//! TODO
int
main
	()
{
	int errCount{ 0 };

//	std::ofstream ofs("/dev/stdout");
std::ofstream ofs("/dev/null");

	constexpr std::size_t numRad{ 33u };
	constexpr std::size_t numPar{ 33u };
//#define UseNorm
#if defined(UseNorm)
	peri::Shape const shape(peri::shape::sWGS84.normalizedShape());
	constexpr double altLo{ -(100./6370.) };
	constexpr double altHi{  (100./6370.) };
#else
	peri::Shape const shape(peri::shape::sWGS84);
	constexpr double altLo{ -100.e+3 };
	constexpr double altHi{  100.e+3 };
#endif

	peri::EarthModel const earth(shape);
	peri::Ellipsoid const & ellip = earth.theEllip;

	double const radEarth{ ellip.lambda() };
	double const radMin{ radEarth + altLo };
	double const radMax{ radEarth + altHi };
	using Range = std::pair<double, double>;
	peri::sim::SampleSpec const radSpec{ numRad, Range{ radMin, radMax } };
	peri::sim::SampleSpec const parSpec{ numPar, Range{ 0.,  .5*peri::pi()} };

	errCount += test1(ofs, radSpec, parSpec, earth, true);
	std::cout << '\n';
	errCount += test2(radSpec, parSpec, earth);
	std::cout << '\n';

	return errCount;
}

