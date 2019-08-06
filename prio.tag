<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile>
  <compound kind="file">
    <name>api.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>api_8h</filename>
    <includes id="common_8h" name="common.h" local="yes" imported="no">priocpp/common.h</includes>
    <includes id="loop_8h" name="loop.h" local="yes" imported="no">priocpp/loop.h</includes>
    <includes id="timeout_8h" name="timeout.h" local="yes" imported="no">priocpp/timeout.h</includes>
    <includes id="signal_8h" name="signal.h" local="yes" imported="no">priocpp/signal.h</includes>
    <includes id="url_8h" name="url.h" local="yes" imported="no">priocpp/url.h</includes>
    <class kind="class">prio::Libraries</class>
    <class kind="class">prio::connection_timeout_t</class>
    <class kind="class">prio::Connection</class>
    <class kind="class">prio::SslCtx</class>
    <class kind="class">prio::Listener</class>
    <class kind="class">prio::IO</class>
    <member kind="function">
      <type>void</type>
      <name>nextTick</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>adda63700d6e78cfc221e3f0a8429489e</anchor>
      <arglist>(const std::function&lt; void()&gt; f) noexcept</arglist>
    </member>
    <member kind="function">
      <type>auto</type>
      <name>resolved</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>a739d2a20b10cee9b47c52c30149d3b89</anchor>
      <arglist>(VArgs ... vargs) noexcept</arglist>
    </member>
    <member kind="function">
      <type>auto</type>
      <name>rejected</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>a1eab27e8fc8b02d394f5979484e652f5</anchor>
      <arglist>(P p, E ex) noexcept</arglist>
    </member>
    <member kind="function">
      <type>auto</type>
      <name>reject</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>aed4423f65e6a716430a4df7a7a4f271a</anchor>
      <arglist>(P p)</arglist>
    </member>
    <member kind="function">
      <type>connection_timeout_t &amp;</type>
      <name>connection_timeouts</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>a5196cb7dfb9e112b12cdc74a58e2ad2d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>SslCtx &amp;</type>
      <name>theSslCtx</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>aea54be739cbf74d51a92f505c354b816</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>repro::Future</type>
      <name>forEach</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>ac7114388e5f5b2bfecdb2bcc49517279</anchor>
      <arglist>(I begin, I end, F f)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future</type>
      <name>forEach</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>a8b6e75a9a909a7bcdefde6ae2d3966fc</anchor>
      <arglist>(C &amp;c, F f)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; Args... &gt;</type>
      <name>future</name>
      <anchorfile>api_8h.html</anchorfile>
      <anchor>a64403ff5d9a7b8a30c3fcdad95820764</anchor>
      <arglist>(T cb)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>common.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>common_8h</filename>
    <class kind="class">prio::IoEx</class>
    <class kind="class">prio::IoErr</class>
    <class kind="class">prio::IoEof</class>
    <class kind="class">prio::IoTimeout</class>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>abefd489a3636506757c4958115b3928d</anchor>
      <arglist>(const std::string &amp;input)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>nonce</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>a4a57efe81693d9a4cfc2a7e21368ab5c</anchor>
      <arglist>(unsigned int n)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>base64_encode</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>ac1f70c790eac5521ab25c62f7a714a84</anchor>
      <arglist>(const std::string &amp;bytes_to_encode)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>base64_encode</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>ae4f14f848f93e5e8b84b4e329fbf628e</anchor>
      <arglist>(unsigned char const *bytes_to_encode, unsigned int in_len)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>base64_decode</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>a0745d2729c17e90802f76f34b16f29cd</anchor>
      <arglist>(const std::string &amp;encoded_string)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>unix_timestamp</name>
      <anchorfile>common_8h.html</anchorfile>
      <anchor>a5cb3ce61e652cf6c1fd19bf72d7956f5</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>loop.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>loop_8h</filename>
    <class kind="class">prio::Loop</class>
    <member kind="function">
      <type>Loop &amp;</type>
      <name>theLoop</name>
      <anchorfile>loop_8h.html</anchorfile>
      <anchor>a9ea1c0beeac8a2c8ce19401d5b6c616f</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pipe.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>pipe_8h</filename>
    <includes id="api_8h" name="api.h" local="yes" imported="no">priocpp/api.h</includes>
    <class kind="class">prio::Pipe</class>
  </compound>
  <compound kind="file">
    <name>signal.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>signal_8h</filename>
    <includes id="common_8h" name="common.h" local="yes" imported="no">priocpp/common.h</includes>
    <member kind="function">
      <type>repro::Future&lt; int &gt;</type>
      <name>signal</name>
      <anchorfile>signal_8h.html</anchorfile>
      <anchor>ae479923d98f41b9e0ae9defa939aad5c</anchor>
      <arglist>(int s) noexcept</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>task.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>task_8h</filename>
    <includes id="api_8h" name="api.h" local="yes" imported="no">priocpp/api.h</includes>
    <member kind="function">
      <type>auto</type>
      <name>task</name>
      <anchorfile>task_8h.html</anchorfile>
      <anchor>ab011f36aed0b849e6c3ba021273ec5b3</anchor>
      <arglist>(T t, ThreadPool &amp;pool=thePool()) -&gt; repro::Future&lt; decltype(t())&gt;</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>timeout.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>timeout_8h</filename>
    <includes id="common_8h" name="common.h" local="yes" imported="no">priocpp/common.h</includes>
    <member kind="function">
      <type>repro::Future</type>
      <name>timeout</name>
      <anchorfile>timeout_8h.html</anchorfile>
      <anchor>af3e729d171f46dad81e5e3e271855bb5</anchor>
      <arglist>(int secs, int ms) noexcept</arglist>
    </member>
    <member kind="function">
      <type>repro::Future</type>
      <name>timeout</name>
      <anchorfile>timeout_8h.html</anchorfile>
      <anchor>ae995e6764846467b8cebc9df6eadad79</anchor>
      <arglist>(int secs) noexcept</arglist>
    </member>
    <member kind="function">
      <type>repro::Future</type>
      <name>nextTick</name>
      <anchorfile>timeout_8h.html</anchorfile>
      <anchor>a9bdfcb5dde1478646fbf24b02910ced2</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>timeout</name>
      <anchorfile>timeout_8h.html</anchorfile>
      <anchor>af8dcdb48379add59f576eec967a6c7c0</anchor>
      <arglist>(const std::function&lt; void()&gt; &amp;f, int secs, int ms) noexcept</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>timeout</name>
      <anchorfile>timeout_8h.html</anchorfile>
      <anchor>a7a7d265faa230038247ae2f07ba33682</anchor>
      <arglist>(const std::function&lt; void()&gt; &amp;f, int secs) noexcept</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>url.h</name>
    <path>/home/mike/workspace/prio/include/priocpp/</path>
    <filename>url_8h</filename>
    <includes id="common_8h" name="common.h" local="yes" imported="no">priocpp/common.h</includes>
    <class kind="class">prio::Url</class>
  </compound>
  <compound kind="class">
    <name>prio::Connection</name>
    <filename>classprio_1_1Connection.html</filename>
    <member kind="function" virtualness="pure">
      <type>virtual repro::Future&lt; Connection::Ptr, std::string &gt;</type>
      <name>read</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>aa79148975e071d97e96e3ef5316e7e2c</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual repro::Future&lt; Connection::Ptr, std::string &gt;</type>
      <name>read</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>ac45f26e95ba34fd61499dd0c011cd053</anchor>
      <arglist>(size_t n)=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual repro::Future&lt; Connection::Ptr &gt;</type>
      <name>write</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>ab99419c678cc6882b3dd0c849f81e459</anchor>
      <arglist>(const std::string &amp;data)=0</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>isHttp2Requested</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>a55d0920bd20340b94c5c9dc34cefcf4e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>close</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>a5ca21e85333339041bb19036ee1cf4c2</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual repro::Future</type>
      <name>shutdown</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>abd120dc0be6fdd8a35f12248b592a4c8</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>cancel</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>abc67acec4cb6284f5398ff62e13edd9d</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual connection_timeout_t &amp;</type>
      <name>timeouts</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>aca565f3321c5af5beed0bceb0ef54111</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static repro::Future&lt; Connection::Ptr &gt;</type>
      <name>connect</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>aaed0a23878a9bf0fea3bf44b5554481f</anchor>
      <arglist>(const std::string &amp;host, int port)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static repro::Future&lt; Connection::Ptr &gt;</type>
      <name>connect</name>
      <anchorfile>classprio_1_1Connection.html</anchorfile>
      <anchor>ad43bf3b114defce07d73a83342d7442d</anchor>
      <arglist>(const std::string &amp;host, int port, SslCtx &amp;ctx)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::connection_timeout_t</name>
    <filename>structprio_1_1connection__timeout__t.html</filename>
    <member kind="variable">
      <type>int</type>
      <name>connect_timeout_s</name>
      <anchorfile>structprio_1_1connection__timeout__t.html</anchorfile>
      <anchor>a7034432781fd802ea961e7b74d8742b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>rw_timeout_s</name>
      <anchorfile>structprio_1_1connection__timeout__t.html</anchorfile>
      <anchor>a7ca82c652c4bf73b959f80e7c8c4c505</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::IO</name>
    <filename>classprio_1_1IO.html</filename>
    <member kind="function">
      <type>repro::Future</type>
      <name>onRead</name>
      <anchorfile>classprio_1_1IO.html</anchorfile>
      <anchor>a8edea57c02f3c70baa69f2d8a22b2ab0</anchor>
      <arglist>(socket_t fd)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future</type>
      <name>onWrite</name>
      <anchorfile>classprio_1_1IO.html</anchorfile>
      <anchor>ac55f1d89fcbc87761f2ab355ac520051</anchor>
      <arglist>(socket_t fd)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>cancel</name>
      <anchorfile>classprio_1_1IO.html</anchorfile>
      <anchor>a0bbf75db552c864ceb800786b7300786</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::IoEof</name>
    <filename>classprio_1_1IoEof.html</filename>
    <base>ReproEx&lt; IoEof &gt;</base>
  </compound>
  <compound kind="class">
    <name>prio::IoErr</name>
    <filename>classprio_1_1IoErr.html</filename>
    <base>ReproEx&lt; IoErr &gt;</base>
  </compound>
  <compound kind="class">
    <name>prio::IoEx</name>
    <filename>classprio_1_1IoEx.html</filename>
    <base>ReproEx&lt; IoEx &gt;</base>
  </compound>
  <compound kind="class">
    <name>prio::IoTimeout</name>
    <filename>classprio_1_1IoTimeout.html</filename>
    <base>ReproEx&lt; IoTimeout &gt;</base>
  </compound>
  <compound kind="class">
    <name>prio::Libraries</name>
    <filename>classprio_1_1Libraries.html</filename>
    <templarg></templarg>
    <templarg>Args</templarg>
  </compound>
  <compound kind="class">
    <name>prio::Listener</name>
    <filename>classprio_1_1Listener.html</filename>
    <member kind="function">
      <type>void</type>
      <name>cancel</name>
      <anchorfile>classprio_1_1Listener.html</anchorfile>
      <anchor>a8f71d3446e6462436fe8cdf1ed072fd1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; ConnectionPtr &gt;</type>
      <name>bind</name>
      <anchorfile>classprio_1_1Listener.html</anchorfile>
      <anchor>ada57f281baf349afca79c01f49c36421</anchor>
      <arglist>(int port)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::Loop</name>
    <filename>classprio_1_1Loop.html</filename>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>run</name>
      <anchorfile>classprio_1_1Loop.html</anchorfile>
      <anchor>afd47242349ee230715b7c8714ac77b52</anchor>
      <arglist>() noexcept=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>exit</name>
      <anchorfile>classprio_1_1Loop.html</anchorfile>
      <anchor>afb0ca68ebaa020f72cff44a5945d8e29</anchor>
      <arglist>() noexcept=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>onThreadStart</name>
      <anchorfile>classprio_1_1Loop.html</anchorfile>
      <anchor>a215c0a856619975b0dd80f551f6d6d50</anchor>
      <arglist>(std::function&lt; void()&gt; f) noexcept=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>onThreadShutdown</name>
      <anchorfile>classprio_1_1Loop.html</anchorfile>
      <anchor>ac94f20ec34b83f125af96f25663e043c</anchor>
      <arglist>(std::function&lt; void()&gt; f) noexcept=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual bool</type>
      <name>isEventThread</name>
      <anchorfile>classprio_1_1Loop.html</anchorfile>
      <anchor>ad51d66b85c051f026a0b3a072b7b9887</anchor>
      <arglist>() const noexcept=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::Pipe</name>
    <filename>classprio_1_1Pipe.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; Pipe &gt;</type>
      <name>Ptr</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a681dc4b4d5e1e9ba79bd4931ac7e546c</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>Ptr</type>
      <name>stdin</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a6ce661467b5e3f6d1b2b626c7d7d9bc5</anchor>
      <arglist>(const std::string &amp;s)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; Pipe::Ptr &gt;</type>
      <name>pipe</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a1069113e5eef8c0b6fc51f4a8b959999</anchor>
      <arglist>(const std::string &amp;path)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; Pipe::Ptr &gt;</type>
      <name>pipe</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a342d759bad0f139de35ad1988c4ec8e9</anchor>
      <arglist>(const std::string &amp;path, A &amp;&amp;a)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; Pipe::Ptr &gt;</type>
      <name>pipe</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a9cdccb49a362820294c76ff3d5b884ae</anchor>
      <arglist>(const std::string &amp;path, A &amp;&amp;a, char **env)</arglist>
    </member>
    <member kind="function">
      <type>repro::Future&lt; Pipe::Ptr &gt;</type>
      <name>pipe</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>ad83ead79c5a0f482e78720be7a0d65fc</anchor>
      <arglist>(const std::string &amp;path, A &amp;&amp;args, E &amp;&amp;env)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>stdout</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a59b6dc1d201da6a1d09e3e16e276e651</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>stderr</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>a1250ba87a2fd6d5fec2dd20d5800617d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>result</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>ac774cab8c6cbe38e98f0637ee37b9f71</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Ptr</type>
      <name>create</name>
      <anchorfile>classprio_1_1Pipe.html</anchorfile>
      <anchor>acd99f582462ac57b7b5ea969633bb3b1</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::SslCtx</name>
    <filename>classprio_1_1SslCtx.html</filename>
    <member kind="function">
      <type>void</type>
      <name>load_cert_pem</name>
      <anchorfile>classprio_1_1SslCtx.html</anchorfile>
      <anchor>aac75b8c7835fcbe7c76bb55a3ec0e131</anchor>
      <arglist>(const std::string &amp;file)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>prio::Url</name>
    <filename>classprio_1_1Url.html</filename>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>prio</title>
    <filename>index</filename>
  </compound>
</tagfile>
