#ifndef CONSTRAINT_UNIQUENESS_H
#define CONSTRAINT_UNIQUENESS_H

#include "constraint.h"

class ConstraintUniqueness : public Constraint
{
public:
    ConstraintUniqueness(unsigned line, unsigned gridSize = 0);

    bool eval(Grid& grid);
    operator QString() const;
    constraint * toC();

private:
    unsigned line;
    unsigned gridSize;
};

inline bool compareDifference (unsigned first, unsigned second)
{
    return  first == 0 || second == 0 || first != second;
}

#endif // CONSTRAINT_UNIQUENESS_H
