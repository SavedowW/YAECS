#ifndef COLLIDER_H_
#define COLLIDER_H_
#include "Vector2.h"

/*
        p0
        |\
        | \
        |  \
        |   \
        |    \
        |     \
size.y  |      | p1
        |      |
        |      | size.y - size.x * topAngle
        |      |
        |      |
     p3 -------- p2
         size.x
*/

struct SlopeCollider
{
    SlopeCollider() = default;
    SlopeCollider(const Vector2<float> &tlPos_, const Vector2<float> &size_, float topAngleCoef_, int obstacleId_ = false);
    SlopeCollider(const Vector2<float> (&vertices_)[4], int obstacleId_ = false);
    Vector2<float> m_tlPos;
    Vector2<float> m_size;
    float m_topAngleCoef = 0.0f;
    float m_highestSlopePoint = 0.0f;
    float m_lowestSlopePoint = 0.0f;
    bool m_hasSlope = false;
    bool m_hasBox = false;

    int m_obstacleId = 0;

    Vector2<float> m_points[4];
    void generatePoints();

    /*
        Return values:
        0 - no overlap
        1 - found overlap with slope part
        2 - found overlap with box part
    */
    template<bool H_OVERLAP_ONLY, bool V_OVERLAP_ONLY, bool PRIORITIZE_BOX = false>
    inline int getFullCollisionWith(const Collider &cld_, float &highestPoint_) const
    {
        auto horRes = (H_OVERLAP_ONLY ? getHorizontalOverlap(cld_) : getHorizontalCollision(cld_));

        float highest = 0;

        if (!horRes)
            return 0;

        if (horRes == 1)
            highest = std::min(m_tlPos.y, m_tlPos.y + m_topAngleCoef * (cld_.x + cld_.w - m_tlPos.x));
        else if (horRes == 2)
            highest = std::min(m_tlPos.y + m_topAngleCoef * m_size.x, m_tlPos.y + m_topAngleCoef * (cld_.x - m_tlPos.x));
        else if (horRes == 3)
            highest = std::min(m_tlPos.y, m_tlPos.y + m_topAngleCoef * m_size.x);
        else if (horRes == 4)
            highest = std::min(m_tlPos.y + m_topAngleCoef * (cld_.x - m_tlPos.x), m_tlPos.y + m_topAngleCoef * (cld_.x + cld_.w - m_tlPos.x));

        if (PRIORITIZE_BOX)
        {
            if (m_hasBox && (V_OVERLAP_ONLY ? 
                    getVerticalOverlap(m_lowestSlopePoint, m_points[3].y, cld_.y, cld_.y + cld_.h) : 
                    getVerticalCollision(m_lowestSlopePoint, m_points[3].y, cld_.y, cld_.y + cld_.h)))
                    {
                        highestPoint_ = m_lowestSlopePoint;
                        return 2;
                    }
                    
            if (m_hasSlope && cld_.y + cld_.h <= m_lowestSlopePoint && (V_OVERLAP_ONLY ? 
                    getVerticalOverlap(highest, m_lowestSlopePoint, cld_.y, cld_.y + cld_.h) : 
                    getVerticalCollision(highest, m_lowestSlopePoint, cld_.y, cld_.y + cld_.h)))
                    {
                        highestPoint_ = highest;
                        return 1;
                    }
        }
        else
        {
            if (m_hasSlope && (V_OVERLAP_ONLY ? 
                    getVerticalOverlap(highest, m_lowestSlopePoint, cld_.y, cld_.y + cld_.h) : 
                    getVerticalCollision(highest, m_lowestSlopePoint, cld_.y, cld_.y + cld_.h)))
                    {
                        highestPoint_ = highest;
                        return 1;
                    }
    
            if (m_hasBox && (V_OVERLAP_ONLY ? 
                    getVerticalOverlap(m_lowestSlopePoint, m_points[3].y, cld_.y, cld_.y + cld_.h) : 
                    getVerticalCollision(m_lowestSlopePoint, m_points[3].y, cld_.y, cld_.y + cld_.h)))
                    {
                        highestPoint_ = m_lowestSlopePoint;
                        return 2;
                    }
        }

        return 0;
    }

    int getHorizontalOverlap(const Collider &cld_) const;
    int getHorizontalCollision(const Collider &cld_) const;

    bool getVerticalOverlap(float l1Highest_, float l1Lowest_, float l2Highest_, float l2Lowest_) const;
    bool getVerticalCollision(float l1Highest_, float l1Lowest_, float l2Highest_, float l2Lowest_) const;

    float getTopCoord() const;
    int getOrientationDir() const;

    float getTopHeight(const Collider &cld_, int horOverlapType_) const;
};


#endif