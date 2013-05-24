
#include <signal.h>
#include "common/base/log.h"
#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/signal_handlers.h"
#include "common/sync/event.h"
#include "common/sync/thread.h"
#include "net/base/selector.h"
#include "net/base/address.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/server/rpc_server.h"
#include "net/rpc/lib/server/rpc_services_manager.h"

#include "../types.h"            // custom types
#include "services_impl.h"       // services implementations

/**********************************************************************/
/*                        Custom RPC Server                           */
/**********************************************************************/
synch::Event * evProgEnd = NULL;
void HandleNiceExit(int signum)
{
  if(signum != SIGINT)
  {
    LOG_ERROR << "HandleNiceExit Erroneus called for " << signum << " - "
              << strsignal(signum);
    return;
  }
  LOG_WARNING << "\033[31m"  // change foreground to red
                 "Cought SIGINT"
                 "\033[0m";  // reset foreground
  if(common::IsApplicationHanging())
  {
    exit(0);
  }
  if(evProgEnd)
  {
    evProgEnd->Signal();
  }
}

int main(int argc, char ** argv)
{
  // - Initialize logger.
  // - Install default signal handlers.
  common::Init(argc, argv);

  // catch Ctrl+C -> SIGINT
  //
  if(SIG_ERR == ::signal(SIGINT, HandleNiceExit))
  {
    LOG_FATAL << "cannot install SIGNAL-handler: "
              << GetLastSystemErrorDescription();
  }

  //create the nice exit event
  //
  synch::Event _evProgEnd(false, true);
  evProgEnd = &_evProgEnd;

  //create services
  //
  ServiceGigel serviceGigel;
  ServiceMitica serviceMitica;

  //register services
  //
  rpc::ServicesManager servicesManager;
  bool success = servicesManager.RegisterService(serviceGigel) &&
                 servicesManager.RegisterService(serviceMitica);
  if(!success)
  {
    LOG_FATAL << "Failed to register gigel & mitica services.";
  }

  //create the rpc-executor
  //
  rpc::ExecutionPool executor(servicesManager);
  success = executor.Start(1);
  if(!success)
  {
    LOG_FATAL << "Failed to start rpc-execution-pool: "
              << GetLastSystemErrorDescription();
  }

  //create the selector
  //
  net::Selector selector;

  //run the selector in a separate thread
  //
  thread::Thread selectorThread(NewCallback(&selector, &net::Selector::Loop));
  selectorThread.SetJoinable();
  selectorThread.Start();

  //create the rpc server
  //
  rpc::Server rpcServer(selector, executor);
  success = rpcServer.Open(net::HostPort(0, 5678), NULL);
  if(!success)
  {
    LOG_ERROR << "Cannot open port 5678";
    common::Exit(-1);
  }
  else
  {
    // Let it run. Wait for Ctrl+C
    //
    LOG_INFO << "Use Ctrl+C to exit program.";
    evProgEnd->Wait();
  }

  // close the rpc server
  //
  rpcServer.Shutdown();

  // stop the selector (auto closes the rtmp server)
  selector.RunInSelectLoop(NewCallback(&selector, &net::Selector::MakeLoopExit));
  selectorThread.Join();

  // stop the rpc-executor
  //
  executor.Stop();

  // unregister services
  //
  servicesManager.UnregisterService(serviceGigel);
  servicesManager.UnregisterService(serviceMitica);

  LOG_INFO << "Bye.";

  common::Exit(0);
  return 0;
}
