#include "Collider.h"

SlopeCollider::SlopeCollider(const Vector2<float> &tlPos_, const Vector2<float> &size_, float topAngleCoef_, int obstacleId_) :
    m_tlPos(tlPos_),
    m_size(size_),
    m_topAngleCoef(std::min(topAngleCoef_, 1.0f)),
    m_obstacleId(obstacleId_)
{
    generatePoints();
}

SlopeCollider::SlopeCollider(const Vector2<float> (&vertices_)[4], int obstacleId_) :
    m_obstacleId(obstacleId_)
{
    for (int i = 0; i < 4; ++i)
        m_points[i] = vertices_[i];

    m_tlPos = m_points[0];
    m_size = m_points[2] - m_points[0];
    m_topAngleCoef = (m_points[1].y - m_points[0].y) / m_size.x;

    m_highestSlopePoint = std::min(m_points[0].y, m_points[1].y);
    m_lowestSlopePoint = std::max(m_points[0].y, m_points[1].y);
    m_hasSlope = m_points[1].y - m_points[0].y;
    m_hasBox = m_lowestSlopePoint - m_points[2].y;
}

void SlopeCollider::generatePoints()
{
    m_points[0] = m_tlPos;
    m_points[1] = m_tlPos + Vector2{m_size.x, m_size.x * m_topAngleCoef};
    m_points[2] = m_tlPos + m_size;
    m_points[3] = m_tlPos + Vector2{0.0f, m_size.y};

    m_highestSlopePoint = std::min(m_points[0].y, m_points[1].y);
    m_lowestSlopePoint = std::max(m_points[0].y, m_points[1].y);
    m_hasSlope = m_points[1].y - m_points[0].y;
    m_hasBox = m_lowestSlopePoint - m_points[2].y;
}

float SlopeCollider::getTopCoord() const
{
    return std::min(m_points[0].y, m_points[1].y);
}

int SlopeCollider::getOrientationDir() const
{
    if (m_topAngleCoef > 0)
        return -1;
    else if (m_topAngleCoef < 0)
        return 1;
    else
        return 0;
}

float SlopeCollider::getTopHeight(const Collider &cld_, int horOverlapType_) const
{
    if (horOverlapType_ == 3) // Second collider's boundaries are outside of this collider's boundaries
    {
        return m_highestSlopePoint;
    }
    else if (horOverlapType_ == 4) // Second collider is inside this collider
    {
        return std::min(m_tlPos.y + m_topAngleCoef * (cld_.x - m_tlPos.x), m_tlPos.y + m_topAngleCoef * (cld_.x + cld_.w - m_tlPos.x));
    }
    else if (horOverlapType_ == 1) // Second collider is on top of 1st vertical line
    {
        return std::min(m_tlPos.y, m_tlPos.y + m_topAngleCoef * (cld_.x + cld_.w - m_tlPos.x));
    }
    else // Second collider is on top of 2nd vertical line
    {
        return std::min(m_tlPos.y + m_topAngleCoef * m_size.x, m_tlPos.y + m_topAngleCoef * (cld_.x - m_tlPos.x));
    }
}

int SlopeCollider::getHorizontalOverlap(const Collider &cld_) const
{
    if (cld_.x <= m_tlPos.x && cld_.x + cld_.w >= m_points[1].x)
		return 3;
    if (cld_.x >= m_tlPos.x && cld_.x + cld_.w <= m_points[1].x)
		return 4;
    if (cld_.x + cld_.w > m_tlPos.x && cld_.x <= m_tlPos.x)
		return 1;
	if (cld_.x + cld_.w >= m_points[1].x && cld_.x < m_points[1].x)
		return 2;
	
    
	return 0;
}

int SlopeCollider::getHorizontalCollision(const Collider &cld_) const
{
    if (cld_.x <= m_tlPos.x && cld_.x + cld_.w >= m_points[1].x)
		return 3;
    if (cld_.x >= m_tlPos.x && cld_.x + cld_.w <= m_points[1].x)
		return 4;
    if (cld_.x + cld_.w >= m_tlPos.x && cld_.x <= m_tlPos.x)
		return 1;
	if (cld_.x + cld_.w >= m_points[1].x && cld_.x <= m_points[1].x)
		return 2;
	else
		return 0;
}

bool SlopeCollider::getVerticalOverlap(float l1Highest_, float l1Lowest_, float l2Highest_, float l2Lowest_) const
{
    return (l2Lowest_ > l1Highest_ && l2Lowest_ <= l1Lowest_) ||
           (l2Highest_ >= l1Highest_ && l2Highest_ < l1Lowest_) ||
           (l2Highest_ <= l1Highest_ && l2Lowest_ >= l1Lowest_);
}

bool SlopeCollider::getVerticalCollision(float l1Highest_, float l1Lowest_, float l2Highest_, float l2Lowest_) const
{
    return (l2Lowest_ >= l1Highest_ && l2Lowest_ <= l1Lowest_) ||
           (l2Highest_ >= l1Highest_ && l2Highest_ <= l1Lowest_) ||
           (l2Highest_ <= l1Highest_ && l2Lowest_ >= l1Lowest_);
}
