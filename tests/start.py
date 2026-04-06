import optparse
import sys
import time
import subprocess
import os
import traceback
import asyncio

import slixmpp
from importlib.machinery import SourceFileLoader
import logging

logging.basicConfig(level=logging.DEBUG,
                    format='%(levelname)-8s %(message)s')
# Enable raw XML stream logging for debugging
xml_logger = logging.getLogger('slixmpp.xmlstream.xmlstream')
xml_logger.setLevel(logging.DEBUG)
# Add a handler to the XML logger if it doesn't have one
if not xml_logger.handlers:
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(message)s'))
    xml_logger.addHandler(handler)

# Utility: wait for a TCP port to become available
import socket as _socket
import time as _time

def wait_for_port(host, port, timeout=10):
    start_time = _time.time()
    while _time.time() - start_time < timeout:
        try:
            with _socket.create_connection((host, port), timeout=1):
                return True
        except (OSError, _socket.error):
            _time.sleep(0.1)
    return False

async def registerXMPPAccount(user, password):
    import threading
    import time
    import queue

    # Queue to get the responder object
    responder_queue = queue.Queue()

    def create_and_run_responder():
        # Create event loop for this thread
        import asyncio
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        # Create responder in this thread
        responder = slixmpp.ClientXMPP(user, password)
        responder.register_plugin('xep_0030')  # Service Discovery
        responder.register_plugin('xep_0077')
        responder['feature_mechanisms'].unencrypted_plain = True

        # Put responder in queue (though it's not used elsewhere)
        responder_queue.put(responder)

        try:
            # slixmpp 1.7.0: connect() returns None and is synchronous
            responder.connect("127.0.0.1", 5222)
            # Process in this thread's event loop
            responder.process(forever=True, timeout=5)
        except Exception as e:
            print(f"connect() failed in thread: {e}")
            # Don't exit here, let main thread handle it

    thread = threading.Thread(target=create_and_run_responder)
    thread.daemon = True
    thread.start()

    # Get responder from queue (though not really needed)
    try:
        responder = responder_queue.get(timeout=5)
    except queue.Empty:
        print("Timeout waiting for responder to be created")

    time.sleep(2)  # Give it time to connect


class BaseTestCase:
    def __init__(self, env):
        self.env = env
        self.tests = []
        self.enabled = True

    async def setup(self):
        pass

    async def teardown(self):
        pass

    def finished(self):
        return True


# Default test case for one client and one responder
class ClientResponderTestCase(BaseTestCase):
    def __init__(self, env, Client, Responder):
        BaseTestCase.__init__(self, env)
        self.env = env
        self.Client = Client
        self.Responder = Responder
        self.enabled = True
        self.responder_thread = None
        self.client_thread = None
        self.responder = None
        self.client = None

    async def setup(self):
        # We'll create and run clients in threads
        import threading
        import time
        import queue

        # Queue to get the responder object from the thread
        responder_queue = queue.Queue()
        responder_event_loop = None

        # Create and run responder in its own thread
        def create_and_run_responder():
            # Create event loop for this thread
            import asyncio
            nonlocal responder_event_loop
            responder_event_loop = asyncio.new_event_loop()
            asyncio.set_event_loop(responder_event_loop)

            # Create responder in this thread (will use this thread's event loop)
            responder = self.Responder(self.env.responder_jid, self.env.responder_password,
                                      self.env.responder_room, self.env.responder_roompassword,
                                      self.env.responder_nick)
            responder.register_plugin('xep_0030')  # Service Discovery
            responder.register_plugin('xep_0045')  # Multi-User Chat
            responder.register_plugin('xep_0199')  # XMPP Ping
            responder['feature_mechanisms'].unencrypted_plain = True

            # Put responder in queue for main thread
            responder_queue.put(responder)

            # For Spectrum tests, always connect to Spectrum on port 5223
            # For backend tests (when responder_password != "password"), connect to real XMPP server
            try:
                if self.env.responder_password != "password":
                    # Backend test - connect to real XMPP server (auto-detect from JID)
                    import re
                    match = re.match(r'[^@]+@([^/]+)', self.env.responder_jid)
                    if match:
                        domain = match.group(1)
                    # Backend test: connecting responder to backend XMPP
                        responder.connect(domain, 5222)
                    else:
                    # Could not extract domain from JID
                        responder.connect("127.0.0.1", 5222)
                else:
                    # Spectrum test - connect to Spectrum
                    # Spectrum test: connecting responder to configured Spectrum host
                    # Set default_domain explicitly to avoid empty DNS queries
                    host = self.env.spectrum.host or "127.0.0.1"
                    port = self.env.spectrum.port or 5223
                    responder.default_domain = host
                    responder._default_domain = host
                    responder.connect(host, port)

                # Process in this thread's event loop
                responder.process(forever=False, timeout=60)
            except Exception as e:
                print(f"Responder connect() failed in thread: {e}")

        # Queue to get the client object from the thread
        client_queue = queue.Queue()
        client_event_loop = None

        # Create and run client in its own thread
        def create_and_run_client():
            # Wait a bit for responder to start
            time.sleep(1)

            # Create event loop for this thread
            import asyncio
            nonlocal client_event_loop
            client_event_loop = asyncio.new_event_loop()
            asyncio.set_event_loop(client_event_loop)

            # Create client in this thread (will use this thread's event loop)
            client = self.Client(self.env.client_jid, self.env.client_password,
                                self.env.client_room, self.env.client_nick)
            client.register_plugin('xep_0030')  # Service Discovery
            client.register_plugin('xep_0045')  # Multi-User Chat
            client.register_plugin('xep_0199')  # XMPP Ping
            client['feature_mechanisms'].unencrypted_plain = True

            # Put client in queue for main thread
            client_queue.put(client)

            # For Spectrum tests, always connect to Spectrum on port 5223
            # For backend tests (when responder_password != "password"), connect to real XMPP server
            try:
                if self.env.responder_password != "password":
                    # Backend test - connect to real XMPP server
                    # Backend test: connecting client to local XMPP
                    client.connect("127.0.0.1", 5222)
                else:
                    # Spectrum test - connect to Spectrum explicitly
                    # Spectrum test: connecting client to configured Spectrum host
                    # Set default_domain explicitly to avoid empty DNS queries
                    host = self.env.spectrum.host or "127.0.0.1"
                    port = self.env.spectrum.port or 5223
                    client.default_domain = host
                    client._default_domain = host
                    client.connect(host, port)

                # Process in this thread's event loop
                client.process(forever=False, timeout=60)
            except Exception as e:
                print(f"Client connect() failed in thread: {e}")

        # Start both threads
        self.responder_thread = threading.Thread(target=create_and_run_responder)
        self.responder_thread.daemon = False  # Not daemon, we'll join it
        self.responder_thread.start()

        self.client_thread = threading.Thread(target=create_and_run_client)
        self.client_thread.daemon = False  # Not daemon, we'll join it
        self.client_thread.start()

        # Get responder and client objects from queues
        try:
            self.responder = responder_queue.get(timeout=5)
        except queue.Empty:
            print("Timeout waiting for responder to be created")
            self.responder = None

        try:
            self.client = client_queue.get(timeout=5)
        except queue.Empty:
            print("Timeout waiting for client to be created")
            self.client = None

        # Give them time to connect
        time.sleep(2)

    def finished(self):
        return self.client.finished and self.responder.finished

    async def teardown(self):
        self.tests = []
        if hasattr(self, 'client') and self.client:
            self.tests += list(self.client.tests.values())
            # Disconnect client - this should cause process() to return
            self.client.disconnect()
        if hasattr(self, 'responder') and self.responder:
            self.tests += list(self.responder.tests.values())
            # Disconnect responder - this should cause process() to return
            self.responder.disconnect()

        # Wait for threads to finish
        if self.client_thread:
            self.client_thread.join(timeout=5)
        if self.responder_thread:
            self.responder_thread.join(timeout=5)

class Params:
    pass

class BaseTest:
    def __init__(self, config, server_mode, room):
        self.config = config
        self.server_mode = server_mode

        # Spectrum connection parameters
        self.spectrum = Params()
        self.spectrum.host = "127.0.0.1"
        self.spectrum.port = 5223
        self.spectrum.hostname = "localhostxmpp"

        # Legacy network connection parameters (default: XMPP)
        self.backend = Params()
        self.backend.host = "127.0.0.1"
        self.backend.port = 5222
        self.backend.hostname = "localhost"

        # User accounts
        self.client_jid = "client@localhost"
        self.client_password = "password"
        self.client_nick = "client"
        self.client_backend_jid = ""
        self.client_backend_password = ""
        self.responder_jid = "responder@localhost"
        self.responder_password = "password"
        self.responder_nick = "responder"
        self.responder_backend_jid = ""
        self.responder_backend_password = ""
        self.room = room
        self.client_room = room
        self.responder_room = room
        self.responder_roompassword = ""

    def skip_test(self, test):
        return False

    async def start_async(self, TestCaseFile):
        # Use ClientResponder by default, but files may declare their own TestCase()s
        try:
            testCase = TestCaseFile.TestCase(self)
        except (NameError, AttributeError):
            testCase = ClientResponderTestCase(self, TestCaseFile.Client, TestCaseFile.Responder)

        # Let the test self-disqualify
        if hasattr(testCase, 'enabled') and not testCase.enabled:
            print("Skipped (self-disqualified).")
            return True

        pre = self.pre_test()
        if asyncio.iscoroutine(pre):
            await pre
        await asyncio.sleep(1)
        try:
            await testCase.setup()
            max_time = 60
            while not testCase.finished() and max_time > 0:
                if callable(getattr(testCase, "step", None)):
                    # Check if step is async
                    if asyncio.iscoroutinefunction(testCase.step):
                        await testCase.step()
                    else:
                        testCase.step()
                await asyncio.sleep(1)
                max_time -= 1
        finally:
            await testCase.teardown()
            post = self.post_test()
            if asyncio.iscoroutine(post):
                await post

        ret = True
        for v in testCase.tests:
            if v[1]:
                print(v[0] + ": PASSED")
            else:
                print(v[0] + ": FAILED")
                ret = False

        if not ret:
            os.system("cat spectrum2.log")

        return ret

    # Synchronous wrapper for backward compatibility
    def start(self, TestCaseFile):
        return asyncio.run(self.start_async(TestCaseFile))

    async def pre_test(self):
        # Start spectrum server asynchronously using asyncio subprocess
        self.spectrum_process = await asyncio.create_subprocess_exec(
            "../../spectrum/src/spectrum2", "-n", "./" + self.config,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        # Wait for spectrum server to be ready on configured port
        started = await asyncio.get_event_loop().run_in_executor(None, wait_for_port, self.spectrum.host, self.spectrum.port, 15)
        if not started:
            raise RuntimeError(f"Spectrum server did not start on {self.spectrum.host}:{self.spectrum.port} within timeout")
        else:
            print(f"DEBUG: Spectrum server is listening on {self.spectrum.host}:{self.spectrum.port}")

    async def post_test(self):
        # Terminate spectrum server process if running
        if hasattr(self, 'spectrum_process') and self.spectrum_process:
            try:
                self.spectrum_process.terminate()
            except Exception:
                pass
            try:
                await asyncio.wait_for(self.spectrum_process.wait(), timeout=5)
            except asyncio.TimeoutError:
                try:
                    self.spectrum_process.kill()
                except Exception:
                    pass
        # Fallback cleanup
        os.system("killall -w spectrum2")

class LibcommuniServerModeSingleServerConf(BaseTest):
    def __init__(self):
        BaseTest.__init__(self, "../libcommuni/irc_test.cfg", True, "#channel@localhost")
        self.directory = "../libcommuni/"

    def skip_test(self, test):
        if test in ["muc_join_nickname_used.py"]:
            return True
        return False

    async def pre_test(self):
        # Start base setup (spectrum) then start ngircd
        await BaseTest.pre_test(self)
        os.system("/usr/sbin/ngircd -f ../libcommuni/ngircd.conf &")

    def post_test(self):
        os.system("killall -w ngircd 2>/dev/null")
        os.system("killall -w spectrum2_libcommuni_backend 2>/dev/null")
        BaseTest.post_test(self)

class LibcommuniServerModeConf(BaseTest):
    def __init__(self):
        BaseTest.__init__(self, "../libcommuni/irc_test2.cfg", True, "#channel%localhost@localhost")
        self.directory = "../libcommuni/"

    async def pre_test(self):
        await BaseTest.pre_test(self)
        os.system("/usr/sbin/ngircd -f ../libcommuni/ngircd.conf &")

    def post_test(self):
        os.system("killall -w ngircd 2>/dev/null")
        os.system("killall -w spectrum2_libcommuni_backend 2>/dev/null")
        BaseTest.post_test(self)

class JabberServerModeConf(BaseTest):
    def __init__(self):
        BaseTest.__init__(self, "../libpurple_jabber/jabber_test.cfg", True, "room%conference.localhost@localhostxmpp")
        self.directory = "../libpurple_jabber/"
        self.client_jid = "client%localhost@localhostxmpp"
        self.client_backend_jid = "client@localhost"
        self.client_backend_password = self.client_password
        self.responder_jid = "responder%localhost@localhostxmpp"
        self.responder_backend_jid = "responder@localhost"
        self.responder_backend_password = self.responder_password

    def skip_test(self, test):
        if test in ["muc_whois.py", "muc_change_topic.py"]:
            return True
        return False

    async def pre_test(self):
        os.system("cp ../libpurple_jabber/prefs.xml ./ -f >/dev/null")
        await BaseTest.pre_test(self)
        os.system("prosody --config ../libpurple_jabber/prosody.cfg.lua >prosody.log &")
        time.sleep(3)
        os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register client%localhost@localhostxmpp client@localhost password 2>/dev/null >/dev/null")
        os.system("../../spectrum_manager/src/spectrum2_manager -c ../libpurple_jabber/manager.conf localhostxmpp register responder%localhost@localhostxmpp responder@localhost password 2>/dev/null >/dev/null")

    def post_test(self):
        os.system("killall -w -r lua.* 2>/dev/null")
        os.system("killall -w spectrum2_libpurple_backend 2>/dev/null")
        BaseTest.post_test(self)

class JabberSlackServerModeConf(BaseTest):
    def __init__(self):
        BaseTest.__init__(self, "../slack_jabber/jabber_slack_test.cfg", True, "room%conference.localhost@localhostxmpp")
        self.directory = "../slack_jabber/"
        self.client_jid = "client@localhost"
        self.client_room = "room@conference.localhost"
        # Implicitly forces responder to connect to slack.com instead of localhost
        # by passing a nonstandard responder_roompassword
        self.responder_jid = "owner@spectrum2tests.xmpp.slack.com"
        self.responder_password = "spectrum2tests.e2zJwtKjLhLmt14VsMKq"
        self.responder_room = "spectrum2_room@conference.spectrum2tests.xmpp.slack.com"
        self.responder_nick = "owner"
        self.responder_roompassword = "spectrum2tests.e2zJwtKjLhLmt14VsMKq"

    def skip_test(self, test):
        os.system("cp ../slack_jabber/slack.sql .")
        if test.find("bad_password") != -1:
            print("Changing password to 'badpassword'")
            os.system("sqlite3 slack.sql \"UPDATE users SET password='badpassword' WHERE id=1\"")
        return False

    async def pre_test(self):
        await BaseTest.pre_test(self)
        os.system("prosody --config ../slack_jabber/prosody.cfg.lua > prosody.log &")

    def post_test(self):
        os.system("killall -w -r lua.* 2>/dev/null")
        os.system("killall -w spectrum2_libpurple_backend 2>/dev/null")
        BaseTest.post_test(self)

configurations = []
configurations.append(LibcommuniServerModeSingleServerConf())
configurations.append(LibcommuniServerModeConf())
configurations.append(JabberServerModeConf())
#configurations.append(JabberSlackServerModeConf())
#configurations.append(TwitterServerModeConf())

exitcode = 0

# Pass file names to run only those tests
test_list = sys.argv[1:]

async def run_all_tests():
    global exitcode
    for conf in configurations:
        for f in os.listdir(conf.directory):
            if (len(test_list) > 0) and not (f in test_list):
                continue
            if not f.endswith(".py") or f == "start.py":
                continue

            if len(sys.argv) == 2 and sys.argv[1] != f:
                continue

            print(conf.__class__.__name__ + ": Starting " + f + " test ...")
            # Modules must have distinct module names or their clases will be merged by loader!
            modulename = (conf.directory+f).replace("/","_").replace("\\","_").replace(".","_")
            test = SourceFileLoader(modulename, conf.directory + f).load_module()

            if conf.skip_test(f):
                print("Skipped.")
                continue
            try:
                ret = await conf.start_async(test)
            except Exception as e:
                print("Test ended in exception.")
                traceback.print_exc()
                exitcode = -1
                return
            if not ret:
                exitcode = -1

if __name__ == "__main__":
    asyncio.run(run_all_tests())
    sys.exit(exitcode)
