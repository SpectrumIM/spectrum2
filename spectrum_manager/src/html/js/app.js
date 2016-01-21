function show_instances() {
	$.get($.cookie("base_location") + "api/v1/instances", function(data) {
		$("#main_content").html("<h2>List of Spectrum 2 instances</h2><table id='main_result'><tr><th>Name<th>Status</th><th>Actions</th></tr>");

		var admin = $.cookie("admin") == "1";
		$.each(data.instances, function(i, instance) {
			if (instance.running) {
				if (admin) {
					var command = instance.running ? "stop" : "start";
				}
				else {
					var command = instance.registered ? "unregister" : "register";
					if (instance.registered) {
						instance.status += "<br/>Registered as " + instance.username;
					}
				}
			}
			else if (admin) {
				var command = "start";
			}
			else {
				var command = "";
			}
			var row = '<tr>'
			row += '<td>' + instance.name + '</td>'
			row += '<td>' + instance.status + '</td>'

			if (command == 'register') {
				row += '<td><a class="button_command" href="' + $.cookie("base_location") + 'instances/register.shtml?id=' + instance.id + '">' + command + '</a>' + '</td></tr>';
				$("#main_result  > tbody:last-child").append(row);
			}
			else if (command == "") {
				row += '<td></td></tr>';
				$("#main_result  > tbody:last-child").append(row);
			}
			else {
				row += '<td><a class="button_command" href="' + $.cookie("base_location") +  'api/v1/instances/' + command + '/' + instance.id + '">' + command + '</a>' + '</td></tr>';
				$("#main_result  > tbody:last-child").append(row);
				$(".button_command").click(function(e) {
					e.preventDefault();
					$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

					var url = $(this).attr('href');
					$.get(url, function(data) {
						show_instances();
					});
				})
			}
		});
	});
}

function show_users() {
	var admin = $.cookie("admin") == "1";
	if (!admin) {
		$("#main_content").html("<h2>List of Spectrum 2 users</h2><p>Only administrator can list the users.</p>");
		return;
	}

	$.get($.cookie("base_location") + "api/v1/users", function(data) {
		$("#main_content").html("<h2>List of Spectrum 2 users</h2><p>You can add new users <a href=\"register.shtml?back_to_list=1\">here</a>.</p><table id='main_result'><tr><th>Name<th>Actions</th></tr>");

		$.each(data.users, function(i, user) {
			var row = '<tr>'
			row += '<td>' + user.username + '</td>'
			row += '<td><a class="button_command" href="' + $.cookie("base_location") +  'api/v1/users/remove/' + user.username + '">remove</a></td></tr>';
			$("#main_result  > tbody:last-child").append(row);
			$(".button_command").click(function(e) {
				e.preventDefault();
				$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

				var url = $(this).attr('href');
				$.get(url, function(data) {
					show_users();
				});
			})
		});
	});
}

function getQueryParams(qs) {
	qs = qs.split('+').join(' ');

	var params = {},
		tokens,
		re = /[?&]?([^=]+)=([^&]*)/g;

	while (tokens = re.exec(qs)) {
		params[decodeURIComponent(tokens[1])] = decodeURIComponent(tokens[2]);
	}

	return params;
}

function fill_instances_register_form() {
	var query = getQueryParams(document.location.search);
	$("#instance").attr("value", query.id);

	$(".button_command").click(function(e) {
		e.preventDefault();
		$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

		var postdata ={
			"jid": $("#jid").val(),
			"uin": $("#uin").val(),
			"password": $("#password").val()
		};

		$.post($.cookie("base_location") + "api/v1/instances/register/" + $("#instance").val(), postdata, function(data) {
			if (data.oauth2_url) {
				window.location.replace(data.oauth2_url);
			}
			else {
				window.location.replace("index.shtml");
			}
		});
	})
	
	$.get($.cookie("base_location") + "api/v1/instances/register_form/" + query.id, function(data) {
		$("#jid_desc").html(data.username_label + ":");
		$("#uin_desc").html(data.legacy_username_label + ":");
		$("#password_desc").html(data.password_label + ":");

		$("#jid").attr("placeholder", data.username_label);
		$("#uin").attr("placeholder", data.legacy_username_label);
		$("#password").attr("placeholder", data.password_label);
	});
}

function fill_users_register_form() {
	$(".button").click(function(e) {
		e.preventDefault();
		$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

		var postdata ={
			"username": $("#username").val(),
			"password": $("#password").val()
		};

		$.post($.cookie("base_location") + "api/v1/users/add", postdata, function(data) {
			var query = getQueryParams(document.location.search);
			if (query.back_to_list == "1") {
				window.location.replace("list.shtml");
			}
			else {
				window.location.replace("../login/");
			}
		});
	})
}

