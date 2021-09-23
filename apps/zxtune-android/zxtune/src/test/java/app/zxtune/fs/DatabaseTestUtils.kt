package app.zxtune.fs

import org.junit.Assert.*
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.verifyNoMoreInteractions

object DatabaseTestUtils {

    internal inline fun <Object, reified Visitor : Any> testQueryObjects(
        addObject: (Int) -> Object,
        queryObjects: (Visitor) -> Boolean,
        checkCountHint: (Visitor, Int) -> Unit,
        checkAccept: (Visitor, Object) -> Unit
    ) {
        val objects = (1..10).map(addObject)

        testVisitor<Visitor> { visitor ->
            assertTrue(queryObjects(visitor))

            inOrder(visitor).run {
                checkCountHint(verify(visitor), 10)
                objects.forEach { checkAccept(verify(visitor), it) }
            }
        }
    }

    internal inline fun <reified Visitor : Any, Group, Object> testCheckObjectGrouping(
        addObject: (Int) -> Object,
        addGroup: (Int) -> Group,
        addObjectToGroup: (Group, Object) -> Unit,
        queryObjects: (Group, Visitor) -> Boolean,
        noinline checkCountHint: ((Visitor, Int) -> Unit)?,
        checkAccept: (Visitor, Object) -> Unit
    ) {
        val objects = (1..10).map(addObject)
        val groups = (10..50 step 10).map(addGroup)

        testVisitor<Visitor> { visitor ->
            groups.forEach { assertFalse(queryObjects(it, visitor)) }
        }

        testVisitor<Visitor> { visitor ->
            addObjectToGroup(groups[0], objects[0])
            addObjectToGroup(groups[1], objects[1])
            addObjectToGroup(groups[1], objects[2])
            addObjectToGroup(groups[2], objects[3])
            addObjectToGroup(groups[2], objects[4])
            addObjectToGroup(groups[2], objects[5])

            groups.forEachIndexed { groupIdx, group ->
                assertEquals(groupIdx < 3, queryObjects(group, visitor))
            }

            inOrder(visitor) {
                checkCountHint?.invoke(verify(visitor), 1)
                checkAccept(verify(visitor), objects[0])

                checkCountHint?.invoke(verify(visitor), 2)
                checkAccept(verify(visitor), objects[1])
                checkAccept(verify(visitor), objects[2])

                checkCountHint?.invoke(verify(visitor), 3)
                checkAccept(verify(visitor), objects[3])
                checkAccept(verify(visitor), objects[4])
                checkAccept(verify(visitor), objects[5])
            }
        }

        // multiple to multiple relation
        testVisitor<Visitor> { visitor ->
            addObjectToGroup(groups[0], objects[2])
            addObjectToGroup(groups[1], objects[0])
            addObjectToGroup(groups[2], objects[6])
            addObjectToGroup(groups[3], objects[8])

            groups.forEachIndexed { groupIdx, group ->
                assertEquals(groupIdx < 4, queryObjects(group, visitor))
            }

            inOrder(visitor) {
                checkCountHint?.invoke(verify(visitor), 2)
                checkAccept(verify(visitor), objects[0])
                checkAccept(verify(visitor), objects[2])

                checkCountHint?.invoke(verify(visitor), 3)
                checkAccept(verify(visitor), objects[0])
                checkAccept(verify(visitor), objects[1])
                checkAccept(verify(visitor), objects[2])

                checkCountHint?.invoke(verify(visitor), 4)
                checkAccept(verify(visitor), objects[3])
                checkAccept(verify(visitor), objects[4])
                checkAccept(verify(visitor), objects[5])
                checkAccept(verify(visitor), objects[6])

                checkCountHint?.invoke(verify(visitor), 1)
                checkAccept(verify(visitor), objects[8])
            }
        }
    }

    internal inline fun <reified Visitor : Any, Group, Object> testFindObjects(
        addObject: (Int) -> Object,
        addGroup: (Int) -> Group,
        addObjectToGroup: (Group, Object) -> Unit,
        findAll: (Visitor) -> Unit,
        findSpecific: (Visitor) -> Object,
        checkCountHint: (Visitor, Int) -> Unit,
        checkAccept: (Visitor, Group, Object) -> Unit
    ) {
        val objects = (1..10).map(addObject)
        val groups = (10..50 step 10).map(addGroup)

        val obj2group = arrayOf(0, 1, 1, 2, 2, 2)

        testVisitor<Visitor> { visitor ->
            findAll(visitor)
        }

        obj2group.forEachIndexed { objIdx, groupIdx ->
            addObjectToGroup(groups[groupIdx], objects[objIdx])
        }

        testVisitor<Visitor> { visitor ->
            findAll(visitor)

            inOrder(visitor) {
                checkCountHint(verify(visitor), obj2group.size)
                obj2group.forEachIndexed { objIdx, groupIdx ->
                    checkAccept(verify(visitor), groups[groupIdx], objects[objIdx])
                }
            }
        }

        testVisitor<Visitor> { visitor ->
            val found = findSpecific(visitor)
            val objIdx = objects.indexOf(found)
            assertTrue(objIdx > 0)
            val groupIdx = obj2group[objIdx]

            inOrder(visitor) {
                checkCountHint(verify(visitor), 1)
                checkAccept(verify(visitor), groups[groupIdx], found)
            }
        }
    }

    internal inline fun <reified V : Any> testVisitor(cmd: (V) -> Unit) = mock<V>().let { visitor ->
        cmd(visitor)
        verifyNoMoreInteractions(visitor)
    }
}
