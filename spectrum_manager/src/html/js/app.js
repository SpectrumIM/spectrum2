function show_instances() {
    $.get("/api/v1/instances", function(data) {
	$("#main_content").html("<h2>List of Spectrum 2 instances</h2><table id='main_result'><tr><th>Hostname<th>Status</th><th>Command</th></tr>");
        $.each(data.instances, function(i, instance) {
		var command = instance.running ? "stop" : "start";
		var row = '<tr><td>'+ instance.name + '</td><td>' + 
		instance.status + '</td><td><a class="button_command" href="/api/v1/instances/' + 
		command + '/' + instance.id + '">' + command + '</a>' + '</td></tr>';
		$("#main_result  > tbody:last-child").append(row);
		$(".button_command").click(function(e) {
			e.preventDefault();
			$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');
			var url = $(this).attr('href');
			$.get(url, function(data) {
			show_instances();				
			});
		})
        });
});
}
