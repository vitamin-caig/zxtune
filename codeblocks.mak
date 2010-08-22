cbp:
	@echo -e "\
	<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n\
	<CodeBlocks_project_file>\n\
		<FileVersion major=\"1\" minor=\"6\" />\n\
		<Project>\n\
			<Option title=\"$(binary_name)\" />\n\
			<Option makefile_is_custom=\"1\" />\n\
			<Build>\n\
				<Target title=\"$(mode)\">\n\
					<Option output=\"$(target)\" prefix_auto=\"0\" extension_auto=\"0\" />\n\
					<Option type=\"1\" />\n\
					<MakeCommands>\n\
						<Build command=\"\x24make -f \x24makefile mode=\x24target\" />\n\
						<CompileFile command=\"\x24make -f \x24makefile \x24file\" />\n\
						<Clean command=\"\x24make -f \x24makefile clean_all mode=\x24target\" />\n\
						<DistClean command=\"\x24make -f \x24makefile clean_all \x24target\" />\n\
						<AskRebuildNeeded command=\"\x24make -q -f \x24makefile mode=\x24target\" />\n\
						<SilentBuild command=\"\x24make -f \x24makefile mode=\x24target\" />\n\
					</MakeCommands>\n\
				</Target>\n\
			</Build>\n\
		</Project>\n\
	</CodeBlocks_project_file>\n" > $(binary_name).cbp
	@echo -e "\
	<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n\
	<CodeBlocks_layout_file>\n\
		<ActiveTarget name=\"$(mode)\" />\n\
	</CodeBlocks_layout_file>\n" > $(binary_name).layout
