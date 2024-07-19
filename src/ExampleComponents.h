#include "Vector2.h"

struct ComponentTransform
{
    ComponentTransform() = default;

    ComponentTransform(const Vector2<float> &pos_, const Vector2<float> &size_) :
        m_pos(pos_), m_size(size_)
    {
        //std::cout << __FUNCTION__ << " args\n";
    }

    ComponentTransform (const ComponentTransform &rhs_) = delete;

    ComponentTransform (ComponentTransform &&rhs_) :
        m_pos(rhs_.m_pos),
        m_size(rhs_.m_size)
    {
        //std::cout << __FUNCTION__ << " move\n";
    }

    ComponentTransform &operator=(const ComponentTransform &rhs_) = delete;

    ComponentTransform &operator=(ComponentTransform &&rhs_)
    {
        m_pos = rhs_.m_pos;
        m_size = rhs_.m_size;
        //std::cout << __FUNCTION__ << " move\n";
        return *this;
    }

    Vector2<float> m_pos;
    Vector2<float> m_size;
};

struct ComponentPhysical
{
    ComponentPhysical() = default;

    ComponentPhysical(const Collider &cld_, float gravity_) :
        m_cld(cld_), m_gravity(gravity_)
    {
        //std::cout << "ComponentPhysical CONSTRUCT ARGUMENTS\n";
    }

    ComponentPhysical (const ComponentPhysical &rhs_) = delete;

    ComponentPhysical (ComponentPhysical &&rhs_) :
        m_cld(rhs_.m_cld),
        m_gravity(rhs_.m_gravity)
    {
        //std::cout << "ComponentPhysical CONSTRUCT MOVE\n";
    }

    ComponentPhysical &operator=(const ComponentPhysical &rhs_) = delete;

    ComponentPhysical &operator=(ComponentPhysical &&rhs_)
    {
        m_cld = rhs_.m_cld;
        m_gravity = rhs_.m_gravity;
        //std::cout << "ComponentPhysical ASSIGN MOVE\n";
        return *this;
    }

    Collider m_cld;
    float m_gravity;
};

struct ComponentCharacter
{
    ComponentCharacter() = default;

    ComponentCharacter(const std::string &name_, int level_) :
        m_name(name_), m_level(level_)
    {
        //std::cout << "ComponentCharacter CONSTRUCT ARGUMENTS\n";
    }

    ComponentCharacter (const ComponentCharacter &rhs_) = delete;

    ComponentCharacter (ComponentCharacter &&rhs_) :
        m_name(rhs_.m_name),
        m_level(rhs_.m_level)
    {
        //std::cout << "ComponentCharacter CONSTRUCT MOVE\n";
    }

    ComponentCharacter &operator=(const ComponentCharacter &rhs_) = delete;

    ComponentCharacter &operator=(ComponentCharacter &&rhs_)
    {
        m_name = rhs_.m_name;
        m_level = rhs_.m_level;
        //std::cout << "ComponentCharacter ASSIGN MOVE\n";
        return *this;
    }

    std::string m_name;
    int m_level;
};


std::ostream &operator<<(std::ostream &os_, const ComponentTransform &comp_)
{
    os_ << "Transform ( pos: {" << comp_.m_pos << "}, size: {" << comp_.m_size << "} )";
    return os_;
}

std::ostream &operator<<(std::ostream &os_, const ComponentPhysical &comp_)
{
    os_ << "Physical ( collider: " << comp_.m_cld << ", gravity: {" << comp_.m_gravity << "} )";
    return os_;
}

std::ostream &operator<<(std::ostream &os_, const ComponentCharacter &comp_)
{
    os_ << "Character ( name: " << comp_.m_name << ", level: {" << comp_.m_level << "} )";
    return os_;
}