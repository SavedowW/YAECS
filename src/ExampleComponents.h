#ifndef EXAMPLE_COMPONENTS_H_
#define EXAMPLE_COMPONENTS_H_
#include "Vector2.h"

/*
    Examples of components
    STL containers can sometimes use copy construction / assignment despite move versions being provided
    Therefore, its better to explicitly delete copy constructor and operator= and specify needed constructors and operations
*/


struct ComponentTransform
{
    ComponentTransform() = default;

    ComponentTransform(const Vector2<float> &pos_, const Vector2<float> &size_) :
        m_pos(pos_), m_size(size_)
    {
    }

    ComponentTransform (const ComponentTransform &rhs_) = delete;
    ComponentTransform (ComponentTransform &&rhs_) = default;
    ComponentTransform &operator=(const ComponentTransform &rhs_) = delete;
    ComponentTransform &operator=(ComponentTransform &&rhs_) = default;

    Vector2<float> m_pos;
    Vector2<float> m_size;

    ORIENTATION m_orientation;
};

struct ComponentPhysical
{
    ComponentPhysical() = default;

    ComponentPhysical(const Collider &cld_, float gravity_) :
        m_cld(cld_), m_gravity(gravity_)
    {
    }

    ComponentPhysical (const ComponentPhysical &rhs_) = delete;
    ComponentPhysical (ComponentPhysical &&rhs_) = default;
    ComponentPhysical &operator=(const ComponentPhysical &rhs_) = delete;
    ComponentPhysical &operator=(ComponentPhysical &&rhs_) = default;

    Collider m_cld;
    float m_gravity;
    Vector2<float> m_velocity;
};

struct ComponentCharacter
{
    ComponentCharacter() = default;

    ComponentCharacter(const std::string &name_, int level_) :
        m_name(name_), m_level(level_)
    {
    }

    ComponentCharacter (const ComponentCharacter &rhs_) = delete;
    ComponentCharacter (ComponentCharacter &&rhs_) = default;
    ComponentCharacter &operator=(const ComponentCharacter &rhs_) = delete;
    ComponentCharacter &operator=(ComponentCharacter &&rhs_) = default;

    std::string m_name;
    int m_level;
};

struct ComponentPlayerInput
{
    ComponentPlayerInput() = default;
    ComponentPlayerInput (const ComponentPlayerInput &rhs_) = delete;
    ComponentPlayerInput (ComponentPlayerInput &&rhs_) = default;
    ComponentPlayerInput &operator=(const ComponentPlayerInput &rhs_) = delete;
    ComponentPlayerInput &operator=(ComponentPlayerInput &&rhs_) = default;

    bool m_inL = false;
    bool m_inR = false;
};

struct ComponentMobNavigation
{
    int dir = 0;
    int framesLeft = 0;
};


inline std::ostream &operator<<(std::ostream &os_, const ComponentTransform &comp_)
{
    os_ << "Transform ( pos: {" << comp_.m_pos << "}, size: {" << comp_.m_size << "} )";
    return os_;
}

inline std::ostream &operator<<(std::ostream &os_, const ComponentPhysical &comp_)
{
    os_ << "Physical ( collider: " << comp_.m_cld << ", gravity: {" << comp_.m_gravity << "}, velocity: {" << comp_.m_velocity << "} )";
    return os_;
}

inline std::ostream &operator<<(std::ostream &os_, const ComponentCharacter &comp_)
{
    os_ << "Character ( name: " << comp_.m_name << ", level: {" << comp_.m_level << "} )";
    return os_;
}

inline std::ostream &operator<<(std::ostream &os_, const ComponentPlayerInput &comp_)
{
    os_ << std::boolalpha << "Inputs ( Left: " << comp_.m_inL << ", Right: " << comp_.m_inR << " )";
    return os_;
}

inline std::ostream &operator<<(std::ostream &os_, const ComponentMobNavigation &comp_)
{
    os_ << std::boolalpha << "MobNav ( dir: " << comp_.dir << ", frames left: " << comp_.framesLeft << " )";
    return os_;
}

#endif
