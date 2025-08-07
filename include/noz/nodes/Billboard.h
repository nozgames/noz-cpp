/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
    class Billboard : public Node3d
    {
    public:
        
        NOZ_DECLARE_TYPEID(Billboard, Node3d)
        
        virtual ~Billboard() = default;

        void lateUpdate() override;

    private:
 
		Billboard();

		void initialize();
        
        void updateBillboard();
        void updateFacingScale();
    };
}