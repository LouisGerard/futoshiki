#include "ai.h"

#include "grid.h"
#include "constraint.h"
#include "assignment.h"
#include <sys\timeb.h>
#include <utility>
#include <vector>
#include <QDebug>

AI::AI(Grid * base, std::vector<Constraint*> constraints, Heuristic *heuristic) :
    base(base),
    constraints(constraints),
    heuristic(heuristic)
{}

int* AI::BackTrackC()
{
    qDebug() << "BT called";
    unsigned gridSize = base->size();
    int* grid = base->toC();

    //constraints
    constraint* constraintsC[constraints.size()];
    for (unsigned i = 0; i < constraints.size(); ++i)
        constraintsC[i] = constraints[i]->toC();

    //vars
    int varStack[gridSize*gridSize];
    int stackTop = -1;

    //domains
    //can be replaced by an array of counters
    int* domains[gridSize*gridSize];
    int* defaultDomain = (int*) malloc(gridSize);
    for (int i = 1; i <= gridSize; ++i)
        *(defaultDomain+i-1) = i;
    int domainsOffsets[gridSize*gridSize];
    for (unsigned i = 0; i < gridSize*gridSize; ++i)
        domainsOffsets[i] = 0;

    //assign
    for (int* item = grid; item-grid < gridSize*gridSize; ++item) {
        if (*item == 0) {
            varStack[++stackTop] = item-grid;
            domains[stackTop] = (int*) malloc(gridSize*sizeof(int));
            memcpy(domains[stackTop], defaultDomain, gridSize*sizeof(int));
        }
    }

    unsigned nbVars = stackTop + 1;

    qDebug() << "Searching...";
    ftime(&start);
    while (true) {
        continue_while:
        if (stackTop == -1) {
            ftime(&stop);
            for (unsigned i = 0; i < nbVars; ++i)
                free(domains[i]);
            return grid;
        }
        if (stackTop == nbVars) {
            ftime(&stop);
            for (unsigned i = 0; i < nbVars; ++i)
                free(domains[i]);
            return nullptr;
        }

        ++iterationsCpt;
        unsigned currentItem = varStack[stackTop];

        //eval domain
        for (; domainsOffsets[stackTop] < gridSize; ++domainsOffsets[stackTop]) {
            *(grid+currentItem) = *(domains[stackTop]+domainsOffsets[stackTop]);
            if (isConsistantC(grid, constraintsC, constraints.size(), gridSize)) {
                --stackTop;
                goto continue_while;
            }
        }

        //reinit domain
        domainsOffsets[stackTop] = 0;

        //reinit grid
        *(grid+currentItem) = 0;

        ++stackTop;
        ++domainsOffsets[stackTop];
    }
}

int *AI::ForwardCheckingC()
{
    qDebug() << "FC called";
    unsigned gridSize = base->size();
    int* grid = base->toC();

    //constraints
    constraint* constraintsC[constraints.size()];
    for (unsigned i = 0; i < constraints.size(); ++i)
        constraintsC[i] = constraints[i]->toC();

    //vars
    int varStack[gridSize*gridSize];
    int stackTop = -1;

    //domains
    int* domains[gridSize*gridSize];
    int* defaultDomain = (int*) malloc(gridSize);
    for (int i = 1; i <= gridSize; ++i)
        *(defaultDomain+i-1) = i;
    int domainsOffsets[gridSize*gridSize];
    for (unsigned i = 0; i < gridSize*gridSize; ++i)
        domainsOffsets[i] = 0;

    //assign
    for (int* item = grid; item-grid < gridSize*gridSize; ++item) {
        if (*item == 0) {
            varStack[++stackTop] = item-grid;
            domains[stackTop] = (int*) malloc(gridSize*sizeof(int));
            memcpy(domains[stackTop], defaultDomain, gridSize*sizeof(int));
        }
    }

    unsigned nbVars = stackTop + 1;

    int domainSizes[nbVars][nbVars];
    for (unsigned i = 0; i < nbVars; ++i)
        for (unsigned j = 0; j < nbVars; ++j)
            domainSizes[i][j] = gridSize;

    qDebug() << "Searching...";
    ftime(&start);
    while (true) {
        continue_while:

        if (stackTop == -1) {
            ftime(&stop);
            for (unsigned i = 0; i < nbVars; ++i)
                free(domains[i]);
            return grid;
        }
        if (stackTop == nbVars) {
            ftime(&stop);
            for (unsigned i = 0; i < nbVars; ++i)
                free(domains[i]);
            return nullptr;
        }
        ++iterationsCpt;

        if (stackTop != nbVars-1)
            memcpy(&domainSizes[stackTop], &domainSizes[stackTop+1], nbVars*sizeof(int));

        unsigned currentItem = varStack[stackTop];

        //eval domain
        for (; domainsOffsets[stackTop] < gridSize; ++domainsOffsets[stackTop]) {
            *(grid+currentItem) = *(domains[stackTop]+domainsOffsets[stackTop]);
            if (isConsistantC(grid, constraintsC, constraints.size(), gridSize)) {
                if (!filterDomainsC(varStack[stackTop--],
                                    grid,
                                    constraintsC,
                                    constraints.size(),
                                    gridSize,
                                    domains,
                                    domainSizes[stackTop]))
                    break;  //go back if one domain is empty
                goto continue_while;
            }
        }

        //reinit domain
        domainsOffsets[stackTop] = 0;

        //reinit grid
        *(grid+currentItem) = 0;

        ++stackTop;
        ++domainsOffsets[stackTop];
    }

}

int *AI::ForwardCheckingCOptimized()
{
    qDebug() << "Not implemented yet";
    return nullptr;
}

bool AI::isConsistantC(int* grid, constraint* constraints[], unsigned consSize, unsigned gridSize)
{
    for (unsigned c = 0; c < consSize; ++c) {
        for (unsigned a = 0; a < constraints[c]->size; ++a) {
            unsigned x1 = *(constraints[c]->array+a*4);
            unsigned y1 = *(constraints[c]->array+a*4+1);
            unsigned x2 = *(constraints[c]->array+a*4+2);
            unsigned y2 = *(constraints[c]->array+a*4+3);
            int item1 = *(grid+y1*gridSize+x1);
            int item2 = *(grid+y2*gridSize+x2);
            ++constraintsCpt;
            if (!constraints[c]->operation(item1, item2))
                return false;
        }
    }
    return true;
}

bool AI::filterDomainsC(int item,
                        int *grid,
                        constraint *constraints[],
                        unsigned consSize,
                        unsigned gridSize,
                        int ** domains,
                        int * domSizes)
{
    int *linkedVal;
    for (unsigned c = 0; c < consSize; ++c) {
        for (unsigned a = 0; a < constraints[c]->size; ++a) {
            unsigned x1 = *(constraints[c]->array+a*4);
            unsigned y1 = *(constraints[c]->array+a*4+1);
            unsigned x2 = *(constraints[c]->array+a*4+2);
            unsigned y2 = *(constraints[c]->array+a*4+3);
            int item1 = y1*gridSize+x1;
            int item2 = y2*gridSize+x2;
            int linkedItem;
            int *val1 = grid+item1;
            int *val2 = grid+item2;
            if (item != item1) {
                if (item != item2)
                    continue;
                linkedItem = item1;
                val1 = (int*) malloc(sizeof(int));
                linkedVal = val1;
            }
            else {
                linkedItem = item2;
                val2 = (int*) malloc(sizeof(int));
                linkedVal = val2;
            }

            if (*linkedVal != 0)
                continue;

            for (unsigned d = 0; d < domSizes[linkedItem]; ++d) {
                ++constraintsCpt;
                *linkedVal = domains[linkedItem][d];
                if (!constraints[c]->operation(*val1, *val2)) {
                    //remove value from domain
                    int temp = domains[linkedItem][d];
                    domains[linkedItem][d] = domains[linkedItem][domSizes[linkedItem]-1];
                    domains[linkedItem][domSizes[linkedItem]-1] = temp;
                    --domSizes[linkedItem];
                }
            }
            if (domSizes[linkedItem] == 0) {
                free(linkedVal);
                return false;
            }
        }
    }
    free(linkedVal);
    return true;
}

struct timeb AI::getStart() const
{
    return start;
}

struct timeb AI::getStop() const
{
    return stop;
}

unsigned AI::getIterationsCpt() const
{
    return iterationsCpt;
}

unsigned AI::getConstraintsCpt() const
{
    return constraintsCpt;
}

