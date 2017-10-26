/* Authors: John Schulman, Bryce Willey */
#include <cmath>
#include <Eigen/Core>

#include "ompl/base/objectives/JointDistanceObjective.h"
#include "ompl/trajopt/expr_ops.h"
#include "ompl/trajopt/utils.h"
#include "ompl/geometric/planners/trajopt/OmplOptProb.h"

/**
 * Takes the difference of each row from the previous row.
 */
static Eigen::MatrixXd diffPrevRow(const Eigen::MatrixXd& in)
{
    // Subtracts rows 0:(end-1) from 1:end-1
    // 2nd arg to `middleRows` is a count, NOT the last index.
    return in.middleRows(1, in.rows() - 1) - in.middleRows(0, in.rows() - 1);
}

ompl::base::JointDistCost::JointDistCost(const trajopt::VarArray& vars) :
    vars_(vars)
{
    for (int i = 0; i < vars.rows() - 1; i++)
    {
        for (int j = 0; j < vars.cols(); j++)
        {
            sco::AffExpr vel;
            sco::exprInc(vel, sco::exprMult(vars(i, j), -1));
            sco::exprInc(vel, vars(i+1, j));
            sco::exprInc(expr_, sco::exprSquare(vel));
        }
    }
}

sco::ConvexObjectivePtr ompl::base::JointDistCost::convex(const std::vector<double>& x, sco::Model* model)
{
    sco::ConvexObjectivePtr out(new sco::ConvexObjective(model));
    out->addQuadExpr(expr_);
    return out;
}

double ompl::base::JointDistCost::value(const std::vector<double>& xvec)
{
    Eigen::MatrixXd traj = trajopt::getTraj(xvec, vars_);
    return (diffPrevRow(traj).array().square().matrix()).sum();
}

/*****************************************
 * OMPL Side of Joint Velocity Objectives.
 ****************************************/

ompl::base::JointDistanceObjective::JointDistanceObjective(const ompl::base::SpaceInformationPtr &si) :
    ConvexifiableObjective(si)
{}

ompl::base::Cost ompl::base::JointDistanceObjective::stateCost(const State *s) const
{
    return identityCost();
}

ompl::base::Cost ompl::base::JointDistanceObjective::motionCost(const State *s1, const State *s2) const
{
    return Cost(pow(si_->distance(s1, s2), 2));
    // TODO: add a coefficient when we know how to separate joints.
    // TODO: if the statespace distance isn't euclidean, then this will give different answers than
    //       the cost pointer.
}

sco::CostPtr ompl::base::JointDistanceObjective::toCost(sco::OptProbPtr problem)
{
    sco::CostPtr cptr(new JointDistCost((std::static_pointer_cast<ompl::geometric::OmplOptProb>(problem)->GetVars())));
    return cptr;
}