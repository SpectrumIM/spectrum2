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
				row += '<td>';
				if (command == 'unregister' && instance.frontend == "slack") {
					row += '<a href="' + $.cookie("base_location") + 'instances/join_room.shtml?id=' + instance.id + '">Join room</a> | ';
					row += '<a href="' + $.cookie("base_location") + 'instances/list_rooms.shtml?id=' + instance.id + '">List joined rooms</a> | ';
				}
				row += '<a class="button_command" href="' + $.cookie("base_location") +  'api/v1/instances/' + command + '/' + instance.id + '">' + command + '</a>';
				row += '</td></tr>';
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

function show_list_rooms() {
	var query = getQueryParams(document.location.search);
	$.get($.cookie("base_location") + "api/v1/instances/list_rooms/" + query.id, function(data) {
		$("#main_content").html("<h2>Joined rooms</h2><table id='main_result'><tr><th>" + data.frontend_room_label + "</th><th>" + data.legacy_room_label + "</th><th>" + data.legacy_server_label + "</th><th>" + data.name_label + "</th><th>Actions</th></tr>");

		$.each(data.rooms, function(i, room) {
			var row = '<tr>';
			row += '<td>' + room.frontend_room + '</td>';
			row += '<td>' + room.legacy_room + '</td>';
			row += '<td>' + room.legacy_server + '</td>';
			row += '<td>' + room.name + '</td>';
			row += '<td><a class="button_command" href="' + $.cookie("base_location") +  'api/v1/instances/leave_room/' + query.id + '?frontend_room=' + room.frontend_room + '">Leave</a></td>';
			row += '</tr>';

			$("#main_result  > tbody:last-child").append(row);
			$(".button_command").click(function(e) {
				e.preventDefault();
				$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

				var url = $(this).attr('href');
				$.get(url, function(data) {
					show_list_rooms();
				});
			})
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

function fill_instances_join_room_form() {
	var query = getQueryParams(document.location.search);
	$("#instance").attr("value", query.id);

	$(".button_command").click(function(e) {
		e.preventDefault();
		$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

		var postdata ={
			"name": $("#name").val(),
			"legacy_room": $("#legacy_room").val(),
			"legacy_server": $("#legacy_server").val(),
			"frontend_room": $("#frontend_room").val()
		};

		$.post($.cookie("base_location") + "api/v1/instances/join_room/" + $("#instance").val(), postdata, function(data) {
			window.location.replace("index.shtml");
		});
	})
	
	$.get($.cookie("base_location") + "api/v1/instances/join_room_form/" + query.id, function(data) {
		$("#name_desc").html(data.name_label + ":");
		$("#legacy_room_desc").html(data.legacy_room_label + ":");
		$("#legacy_server_desc").html(data.legacy_server_label + ":");
		$("#frontend_room_desc").html(data.frontend_room_label + ":");

		$("#name").attr("placeholder", data.name_label + ":");
		$("#legacy_room").attr("placeholder", data.legacy_room_label + ":");
		$("#legacy_server").attr("placeholder", data.legacy_server_label + ":");
		$("#frontend_room").attr("placeholder", data.frontend_room_label + ":");
	});
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

		var postdata ={
			"username": $("#username").val(),
			"password": $("#password").val()
		};

		$.post("/api/v1/users/add", postdata, function(data) {
			if (data.error) {
				$('#error').text(data.message);
			}
			else {
				var query = getQueryParams(document.location.search);
				if (query.back_to_list == "1") {
					window.location.replace("list.shtml");
				}
				else {
					window.location.replace("../login/");
				}
			}
		});
	})
}

