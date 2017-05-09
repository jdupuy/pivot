#include <mitsuba/render/phase.h>
#include <mitsuba/render/sampler.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/frame.h>
#include <mitsuba/core/warp.h>

MTS_NAMESPACE_BEGIN

/*!\plugin{hg}{Pivot phase function}
 * \order{2}
 * \parameters{
 *     \parameter{g}{\Float}{
 *       This parameter must be somewhere in the range $-1$ to $1$
 *       (but not equal to $-1$ or $1$). It denotes the \emph{mean cosine}
 *       of scattering interactions. A value greater than zero indicates that
 *       medium interactions predominantly scatter incident light into a similar
 *       direction (i.e. the medium is \emph{forward-scattering}), whereas
 *       values smaller than zero cause the medium to be
 *       scatter more light in the opposite direction.
 *     }
 * }
 * This plugin implements the phase function model proposed by
 * Dupuy et al.. It is parameterizable from backward- ($g<0$) through
 * isotropic- ($g=0$) to forward ($g>0$) scattering.
 */
class PivotPhaseFunction : public PhaseFunction {
public:
	PivotPhaseFunction(const Properties &props)
		: PhaseFunction(props) {
		/* Asymmetry parameter: must lie in [-1, 1] where >0 is
		   forward scattering and <0 is backward scattering. */
		m_g = props.getFloat("g", 0.8f);
		if (m_g >= 1 || m_g <= -1)
			Log(EError, "The asymmetry parameter must lie in the interval (-1, 1)!");
	}

	PivotPhaseFunction(Stream *stream, InstanceManager *manager)
		: PhaseFunction(stream, manager) {
		m_g = stream->readFloat();
		configure();
	}

	virtual ~PivotPhaseFunction() { }

	void serialize(Stream *stream, InstanceManager *manager) const {
		PhaseFunction::serialize(stream, manager);

		stream->writeFloat(m_g);
	}

	void configure() {
		PhaseFunction::configure();
		m_type = EAngleDependence;
	}

	inline Vector project(const Vector& std, const Vector& pivot) const {
		Vector tmp = std - pivot;
		Vector cp1 = cross(std, pivot);
		Vector cp2 = cross(tmp, cp1);
		Float dp = dot(std, pivot) - 1.0;
		Float qf = dp * dp + dot(cp1, cp1);

		return ((dp * tmp - cp2) / qf);
	}

	inline Float sample(PhaseFunctionSamplingRecord &pRec,
			Sampler *sampler) const {
		Point2 sample(sampler->next2D());
		Vector std = warp::squareToUniformSphere(sample);

		pRec.wo = Frame(-pRec.wi).toWorld(project(std, Vector(0, 0, m_g)));

		return 1.0f;
	}

	Float sample(PhaseFunctionSamplingRecord &pRec,
			Float &pdf, Sampler *sampler) const {
		PivotPhaseFunction::sample(pRec, sampler);
		pdf = PivotPhaseFunction::eval(pRec);
		return 1.0f;
	}

	Float eval(const PhaseFunctionSamplingRecord &pRec) const {
		Float temp1 = 1.0f + m_g * m_g + 2.0f * m_g * dot(pRec.wi, pRec.wo);
		Float temp2 = (1 - m_g * m_g) / temp1;
		return INV_FOURPI * (temp2 * temp2);
	}

	Float getMeanCosine() const {
		return m_g;
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << "PivotPhaseFunction[g=" << m_g << "]";
		return oss.str();
	}

	MTS_DECLARE_CLASS()
private:
	Float m_g;
};

MTS_IMPLEMENT_CLASS_S(PivotPhaseFunction, false, PhaseFunction)
MTS_EXPORT_PLUGIN(PivotPhaseFunction, "Pivot phase function");
MTS_NAMESPACE_END
