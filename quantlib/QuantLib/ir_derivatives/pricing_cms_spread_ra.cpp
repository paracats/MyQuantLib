#include "pricing_cms_spread_ra.hpp"
#include <ir_derivatives\fdg2_cms_spread_ra_engine.hpp>
#include <ir_derivatives\g2_calibration.hpp>


namespace QuantLib {

	std::vector<Real> cms_spread_rangeaccrual_fdm(Date evaluationDate,
		Real notional,
		std::vector<Rate> couponRate,
		std::vector<Real> gearing,
		Schedule schedule,
		DayCounter dayCounter,
		BusinessDayConvention bdc,
		std::vector<Size> tenorsInYears,
		std::vector<Real> lowerBound,
		std::vector<Real> upperBound,
		Date firstCallDate,
		Real pastAccrual,
		boost::shared_ptr<YieldTermStructure> obs1Curve,
		const G2Parameters& obs1G2Params,
		Real obs1FXVol, Real obs1FXCorr,
		Handle<YieldTermStructure>& discTS,
		Size tGrid, Size rGrid,
		Real alpha,
		Real pastFixing,
		Schedule floatingSchedule) {

		QL_REQUIRE(tenorsInYears.size() == 2, "Tenors of CMS spread should be two.");

		Date todaysDate = evaluationDate;
		Settings::instance().evaluationDate() = todaysDate;

		boost::shared_ptr<IborIndex> index(new USDLibor(Period(3,Months), Handle<YieldTermStructure>(discTS)));
		std::vector<Real> gearingVec(gearing);
		std::vector<Real> spread(couponRate);
		std::vector<Real> lowerTrigger1(lowerBound);
		std::vector<Real> upperTrigger1(upperBound);
		
		// defining the models
		Handle<YieldTermStructure> refCurve = Handle<YieldTermStructure>(obs1Curve);
		boost::shared_ptr<G2> modelG2(new G2(refCurve, obs1G2Params.a, obs1G2Params.sigma, obs1G2Params.b, obs1G2Params.eta, obs1G2Params.rho));

		// Define the swaps
		VanillaSwap::Type type = VanillaSwap::Payer;
		boost::shared_ptr<SwapIndex> si1(new UsdLiborSwapIsdaFixPm(Period(tenorsInYears[0], Years), refCurve));
		boost::shared_ptr<SwapIndex> si2(new UsdLiborSwapIsdaFixPm(Period(tenorsInYears[1], Years), refCurve));
		boost::shared_ptr<SwapSpreadIndex> swapSpreadIndex(new SwapSpreadIndex("CMS_spread", si1, si2));
		boost::shared_ptr<FloatFloatSwap> ffSwap(new FloatFloatSwap(
			type, notional, notional, schedule, swapSpreadIndex, dayCounter, floatingSchedule, index, index->dayCounter()));

		// ATM Bermudan swaption pricing
		std::vector<Date> bermudanDates;
		for (Size i = 1; i < schedule.size(); i++) {
			if (schedule[i] >= firstCallDate)
				bermudanDates.push_back(schedule[i]);
		}
		boost::shared_ptr<Exercise> bermudanExercise(new BermudanExercise(bermudanDates));
		FloatFloatSwaption bermudanSwaption(ffSwap, bermudanExercise);


		/////TEMP
		//boost::shared_ptr<SwaptionVolatilityStructure> swaptionVol(new ConstantSwaptionVolatility(evaluationDate, NullCalendar(), Unadjusted, 0.15, Actual365Fixed()));
		bermudanSwaption.setPricingEngine(boost::shared_ptr<PricingEngine>(new FdG2CmsSpreadRAEngine(modelG2, tGrid, rGrid, rGrid)));

		std::vector<Real> rst;
		rst.push_back(bermudanSwaption.NPV());
		return rst;
	}

}