#ifndef PIPELINEOPTIMIZER_H
#define PIPELINEOPTIMIZER_H

#include "pipelineparameters.h"
#include "pipelinecommon.h"
#include <QVector>
#include <QtGlobal> // For M_PI


class PipelineOptimizer {
public:
    QVector<ValidationResult> calculate(const PipelineParameters& params);
};

#endif // PIPELINEOPTIMIZER_H
