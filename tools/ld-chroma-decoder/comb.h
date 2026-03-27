/************************************************************************

    comb.h

    ld-chroma-decoder - Colourisation filter for ld-decode
    Copyright (C) 2018 Chad Page
    Copyright (C) 2018-2019 Simon Inns
    Copyright (C) 2020-2021 Adam Sampson
    Copyright (C) 2021 Phillip Blucas

    This file is part of ld-decode-tools.

************************************************************************/

#ifndef COMB_H
#define COMB_H

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QtMath>
#include <vector> 

#include "lddecodemetadata.h"

#include "componentframe.h"
#include "decoder.h"
#include "sourcefield.h"
#include <fstream>
#include <mutex>
#include <cstdlib>

class Comb
{
public:
    Comb();

    struct Configuration {
        double chromaGain = 1.0;
        double chromaPhase = 0.0;
        qint32 dimensions = 2;
        bool adaptive = true;
        bool showMap = false;
        bool phaseCompensation = false;

        double cNRLevel = 0.0;
        double yNRLevel = 0.0;

        double chromaWeight = 1.4;
        double adaptThreshold = 0.1;

        qint32 getLookBehind() const;
        qint32 getLookAhead() const;
    };

    const Configuration &getConfiguration() const;
    void updateConfiguration(const LdDecodeMetaData::VideoParameters &videoParameters,
                             const Configuration &configuration);

    void decodeFrames(const QVector<SourceField> &inputFields, qint32 startIndex, qint32 endIndex,
                      QVector<ComponentFrame> &componentFrames);

    static constexpr qint32 MAX_WIDTH = 910;
    static constexpr qint32 MAX_HEIGHT = 525;

private:
    bool configurationSet;
    Configuration configuration;
    LdDecodeMetaData::VideoParameters videoParameters;

    class FrameBuffer {
    public:
        FrameBuffer(const LdDecodeMetaData::VideoParameters &videoParameters_, const Configuration &configuration_);

        void loadFields(const SourceField &firstField, const SourceField &secondField);

        // OLA Accumulators
        std::vector<std::vector<double>> accChroma;
        std::vector<std::vector<double>> weightSum;

        void split1D();
        void split2D();
        
        // [FIX] Adjusted for 4-Field Block (Current + Next)
        void split3D(FrameBuffer &nextFrame,int frameIdx);

        void setComponentFrame(ComponentFrame &_componentFrame) {
            componentFrame = &_componentFrame;
        }

        void finalizeOLA();

        void splitIQ();
        void splitIQlocked();
        void filterIQ();
        void adjustY();
        void doCNR();
        void doYNR();
        void transformIQ(double chromaGain, double chromaPhase);

        void overlayMap(const FrameBuffer &previousFrame, const FrameBuffer &nextFrame);

    private:
        const LdDecodeMetaData::VideoParameters &videoParameters;
        const Configuration &configuration;

        qint32 frameHeight;
        double irescale;
        SourceVideo::Data rawbuffer;
        qint32 firstFieldPhaseID;
        qint32 secondFieldPhaseID;

        struct Sample {
            double pixel[MAX_HEIGHT][MAX_WIDTH];
        } clpbuffer[3];

        struct Candidate {
            double penalty;
            double sample;
        };

        ComponentFrame *componentFrame;

        inline qint32 getFieldID(qint32 lineNumber) const;
        inline bool getLinePhase(qint32 lineNumber) const;
        void getBestCandidate(qint32 lineNumber, qint32 h,
                              const FrameBuffer &previousFrame, const FrameBuffer &nextFrame,
                              qint32 &bestIndex, double &bestSample) const;
        Candidate getCandidate(qint32 refLineNumber, qint32 refH,
                               const FrameBuffer &frameBuffer, qint32 lineNumber, qint32 h,
                               double adjustPenalty) const;
    };
};

#endif // COMB_H
