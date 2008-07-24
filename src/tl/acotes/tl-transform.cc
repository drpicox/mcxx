#include "tl-transform.h"

#include "tl-acotestransform.h"
#include "tl-finalizertransform.h"
#include "tl-forreplicatetransform.h"
#include "tl-initializertransform.h"
#include "tl-peektransform.h"
#include "tl-porttransform.h"
#include "tl-portconnectiontransform.h"
#include "tl-sharedtransform.h"
#include "tl-statetransform.h"
#include "tl-tasktransform.h"
#include "tl-taskgrouptransform.h"
#include "tl-teamreplicatetransform.h"
#include "tl-userporttransform.h"
#include "tl-variabletransform.h"


namespace TL { namespace Acotes {
    
    /* ****************************************************************
     * * Tribal behaviour
     * ****************************************************************/

    std::map<std::string,Transform*> Transform::instanceMap;
    
    Transform* Transform::I(const std::string& driver)
    {
        if (instanceMap.count("default") == 0) {
            instanceMap["default"]= new Transform("default");
        }
        
        Transform* result;
        if (instanceMap.count(driver) != 0) {
            result= instanceMap[driver];
        } else {
            result= NULL;
        }
        
        return result;
    }
    
    /* ****************************************************************
     * * Constructor
     * ****************************************************************/

    Transform::Transform(const std::string& n) : driver(n)
            , _acotes(NULL)
            , _finalizer(NULL)
            , _forReplicate(NULL)
            , _initializer(NULL)
            , _peek(NULL)
            , _port(NULL)
            , _portConnection(NULL)
            , _shared(NULL)
            , _state(NULL)
            , _task(NULL)
            , _taskgroup(NULL)
            , _teamReplicate(NULL)
            , _userPort(NULL)
            , _variable(NULL)
    {
    }
    
    
    
    /* ****************************************************************
     * * Transform behaviour
     * ****************************************************************/

    
    AcotesTransform* Transform::acotes()
    {
        if (!_acotes) {
            _acotes= new AcotesTransform(driver);
        }
    
        return _acotes;
    }

    
    FinalizerTransform* Transform::finalizer()
    {
        if (!_finalizer) {
            _finalizer= new FinalizerTransform(driver);
        }
    
        return _finalizer;
    }

    
    ForReplicateTransform* Transform::forReplicate()
    {
        if (!_forReplicate) {
            _forReplicate= new ForReplicateTransform(driver);
        }
    
        return _forReplicate;
    }

    
    InitializerTransform* Transform::initializer()
    {
        if (!_initializer) {
            _initializer= new InitializerTransform(driver);
        }
    
        return _initializer;
    }

    
    PeekTransform* Transform::peek()
    {
        if (!_peek) {
            _peek= new PeekTransform(driver);
        }
    
        return _peek;
    }
    
    PortTransform* Transform::port()
    {
        if (!_port) {
            _port= new PortTransform(driver);
        }
        
        return _port;
    }
    
    PortConnectionTransform* Transform::portConnection()
    {
        if (!_portConnection) {
            _portConnection= new PortConnectionTransform(driver);
        }
        
        return _portConnection;
    }
    
    SharedTransform* Transform::shared()
    {
        if (!_shared) {
            _shared= new SharedTransform(driver);
        }
        
        return _shared;
    }

    StateTransform* Transform::state()
    {
        if (!_state) {
            _state= new StateTransform(driver);
        }
        
        return _state;
    }
    
    TaskTransform* Transform::task()
    {
        if (!_task) {
            _task= new TaskTransform(driver);
        }
    
        return _task;
    }

    
    TaskgroupTransform* Transform::taskgroup()
    {
        if (!_taskgroup) {
            _taskgroup= new TaskgroupTransform(driver);
        }
    
        return _taskgroup;
    }
    
    TeamReplicateTransform* Transform::teamReplicate()
    {
        if (!_teamReplicate) {
            _teamReplicate= new TeamReplicateTransform(driver);
        }
        
        return _teamReplicate;
    }
    
    UserPortTransform* Transform::userPort()
    {
        if (!_userPort) {
            _userPort= new UserPortTransform(driver);
        }
        
        return _userPort;
    }
    
    VariableTransform* Transform::variable()
    {
        if (!_variable) {
            _variable= new VariableTransform(driver);
        }
        
        return _variable;
    }
    
} /* end namespace Acotes */ } /* end namespace TL */