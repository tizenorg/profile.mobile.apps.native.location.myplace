# for i18n
SET(POFILES ar.po az.po bg.po ca.po cs.po da.po de.po el.po en_GB.po en_PH.po en_US.po es.po es_MX.po es_US.po et.po eu.po fi.po fr.po fr_CA.po ga.po gl.po hi.po hr.po hu.po hy.po is.po it.po ja.po ka.po kk.po ko.po lt.po lv.po mk.po nl.po nb.po pl.po pt.po pt_BR.po ro.po ru.po sk.po sl.po sr.po sv.po tr.po uk.po uz.po zh_CN.po zh_HK.po zh_SG.po zh_TW.po)

SET(MSGFMT "/usr/bin/msgfmt")

FOREACH(pofile ${POFILES})
	SET(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${pofile})
	MESSAGE("PO: ${pofile}")
	GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
	GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)
	SET(moFile ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
	ADD_CUSTOM_COMMAND(
			OUTPUT ${moFile}
			COMMAND ${MSGFMT} -o ${moFile} ${absPofile}
			DEPENDS ${absPofile}
	)
	INSTALL(FILES ${moFile}
		DESTINATION res/locale/${lang}/LC_MESSAGES RENAME ${PKG_NAME}.mo)
	SET(moFiles ${moFiles} ${moFile})
ENDFOREACH(pofile)

MESSAGE(".mo files: ${moFiles}")
ADD_CUSTOM_TARGET(po ALL DEPENDS ${moFiles})

